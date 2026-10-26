#!/usr/bin/env python3
"""Turn photos of people into face textures for the player blobs.

For every photo the script
 1. finds the face and its 478 landmarks (MediaPipe face landmarker). Photos from phone
    videos are often sideways, so it also tries the photo turned by 90, 180 and 270
    degrees and keeps the most upright detection of the most central face,
 2. estimates the head pose: roll from the eye line, yaw and pitch from the face
    landmarker's transformation matrix,
 3. turns the face upright and crops the head. The size comes from the face outline,
    corrected for yaw (narrower face) and pitch (shorter face), so a turned head ends
    up as big as a frontal one,
 4. cuts out face, hair and headwear (MediaPipe multiclass selfie segmenter, limited to
    the region around the face), so the blob keeps the colour of the player around the
    head, and evens out coloured lighting on the skin,
 5. writes a square PNG with transparency and adds the player to players.txt.

The game wraps the square texture onto an ellipse of 65 x 89 pixels (see
RenderManager::createSkinFrame), so the head is cropped with that aspect ratio and
stretched to the square; on the blob it gets its natural proportions back.

Usage:
    prepare_faces.py PHOTO_OR_FOLDER... [--out data/players] [--names names.txt]
                     [--preview sheet.jpg]

names.txt maps photos to player names, one "photo;Name" per line, where photo is the
start of the photo's file name (e.g. "person_001;Anna"). Photos without a name are
called after their file name ("person_001_..." becomes "Person 1").

Needs Python 3.9 - 3.12 with mediapipe, opencv and numpy (see requirements.txt). The two
models are downloaded on the first run to ~/.cache/blobby-knockout.
"""
import argparse
import math
import re
import sys
import unicodedata
import urllib.request
from pathlib import Path

import cv2
import mediapipe as mp
import numpy as np
from mediapipe.tasks.python import BaseOptions, vision

MODELS = {
	"face_landmarker.task": "https://storage.googleapis.com/mediapipe-models/face_landmarker/"
	                        "face_landmarker/float16/latest/face_landmarker.task",
	"selfie_multiclass_256x256.tflite": "https://storage.googleapis.com/mediapipe-models/"
	                                    "image_segmenter/selfie_multiclass_256x256/float32/latest/"
	                                    "selfie_multiclass_256x256.tflite",
}

# classes of the multiclass selfie segmenter
BACKGROUND, HAIR, BODY_SKIN, FACE_SKIN, CLOTHES, ACCESSORIES = range(6)

# landmarks of the face mesh
FACE_OVAL = sorted({i for c in vision.FaceLandmarksConnections.FACE_LANDMARKS_FACE_OVAL
                    for i in (c.start, c.end)})
LEFT_EYE = [33, 133]     # corners of the eye on the left of the image
RIGHT_EYE = [362, 263]
FOREHEAD, CHIN = 10, 152

TEXTURE_SIZE = 256
BLOB_ASPECT = 65 / 89            # width / height of the face ellipse on the blob
OVAL_ASPECT = 0.80               # width / height of the face outline of a frontal face
HEAD_HEIGHT = 1.80               # crop height, in face heights (forehead to chin)
HEAD_RAISE = 0.12                # crop centre above the face centre, in face heights
SKIN_TONE = np.array([0.80, 0.60, 0.50])   # average skin colour (RGB) to correct towards


def download_models(folder):
	folder.mkdir(parents=True, exist_ok=True)
	for name, url in MODELS.items():
		if not (folder / name).exists():
			print(f"downloading {name}")
			urllib.request.urlretrieve(url, folder / name)


class FaceCutter:
	def __init__(self, model_folder):
		self.landmarker = vision.FaceLandmarker.create_from_options(vision.FaceLandmarkerOptions(
			base_options=BaseOptions(model_asset_path=str(model_folder / "face_landmarker.task")),
			num_faces=4, min_face_detection_confidence=0.3, min_face_presence_confidence=0.3,
			output_facial_transformation_matrixes=True))
		self.segmenter = vision.ImageSegmenter.create_from_options(vision.ImageSegmenterOptions(
			base_options=BaseOptions(model_asset_path=str(model_folder / "selfie_multiclass_256x256.tflite")),
			output_confidence_masks=True, output_category_mask=False))

	def find_face(self, image):
		"""The most central face of the photo, upright. Returns (turned image, landmark
		pixels, transformation matrix) or None.
		The landmarker also fits a mesh to a tilted or upside down face, then often the
		wrong way round, so the photo is turned as little as possible: in steps of 30
		degrees either way until the face is found nearly upright, upside down last."""
		fallback = None
		for angle in (0, -30, 30, -60, 60, -90, 90, -120, 120, -150, 150, 180):
			turned = turn(image, angle)
			h, w = turned.shape[:2]
			result = self.landmarker.detect(mp.Image(image_format=mp.ImageFormat.SRGB, data=turned))
			best = None
			for landmarks, matrix in zip(result.face_landmarks, result.facial_transformation_matrixes):
				points = np.array([(p.x * w, p.y * h) for p in landmarks])
				size = np.ptp(points[FACE_OVAL], axis=0).max()
				# the photos are cut around the person: prefer big faces near the middle
				offset = np.linalg.norm(points[FACE_OVAL].mean(axis=0) - (w / 2, h / 2)) / max(w, h)
				score = size * math.exp(-(offset / 0.25) ** 2)
				if best is None or score > best[0]:
					best = (score, turned, points, np.array(matrix))
			if best is None:
				continue
			if abs(eye_roll(best[2])) < math.radians(25):
				return best[1:]
			fallback = fallback or best
		return fallback[1:] if fallback else None

	def cut(self, image, color_fix=0.8):
		"""Texture (RGBA, TEXTURE_SIZE square) and pose info for the face in image (RGB)."""
		found = self.find_face(image)
		if found is None:
			return None, None
		image, points, matrix = found
		roll = eye_roll(points)
		yaw, pitch = matrix_yaw_pitch(matrix)

		# upright face: rotate around the eyes
		eyes = (points[LEFT_EYE].mean(axis=0) + points[RIGHT_EYE].mean(axis=0)) / 2
		rotation = cv2.getRotationMatrix2D(tuple(eyes), math.degrees(roll), 1.0)
		upright = apply(rotation, points)
		oval = upright[FACE_OVAL]
		(x0, y0), (x1, y1) = oval.min(axis=0), oval.max(axis=0)
		# face height from the outline, corrected for the pose: a turned head looks narrower,
		# a nodding head shorter
		height = (y1 - y0) / max(math.cos(pitch), 0.7)
		width = (x1 - x0) / max(math.cos(yaw), 0.7)
		face = (height + width / OVAL_ASPECT) / 2
		centre = np.array([(x0 + x1) / 2, (y0 + y1) / 2 - HEAD_RAISE * face])

		# work canvas: the upright head with room around it, for the segmenter
		canvas_size = 512
		canvas_span = 2.0 * HEAD_HEIGHT * face
		scale = canvas_size / canvas_span
		to_canvas = np.vstack([rotation, [0, 0, 1]])
		to_canvas = np.array([[scale, 0, canvas_size / 2 - scale * centre[0]],
		                      [0, scale, canvas_size / 2 - scale * centre[1]],
		                      [0, 0, 1]]) @ to_canvas
		canvas = cv2.warpAffine(image, to_canvas[:2], (canvas_size, canvas_size),
		                        flags=cv2.INTER_AREA if scale < 1 else cv2.INTER_CUBIC,
		                        borderMode=cv2.BORDER_REPLICATE)
		canvas_points = apply(to_canvas[:2], points)

		alpha, skin = self.head_mask(canvas, canvas_points, face * scale)
		canvas = even_lighting(canvas, skin, color_fix)

		# the crop: blob aspect, stretched to a square
		crop_h = HEAD_HEIGHT * face * scale
		crop_w = crop_h * BLOB_ASPECT
		c = canvas_size / 2
		src = np.float32([[c - crop_w / 2, c - crop_h / 2], [c + crop_w / 2, c - crop_h / 2], [c - crop_w / 2, c + crop_h / 2]])
		dst = np.float32([[0, 0], [TEXTURE_SIZE, 0], [0, TEXTURE_SIZE]])
		to_texture = cv2.getAffineTransform(src, dst)
		rgba = np.dstack([canvas, (alpha * 255).astype(np.uint8)])
		texture = cv2.warpAffine(rgba, to_texture, (TEXTURE_SIZE, TEXTURE_SIZE), flags=cv2.INTER_AREA,
		                         borderMode=cv2.BORDER_CONSTANT, borderValue=(0, 0, 0, 0))
		pose = {"roll": math.degrees(roll), "yaw": math.degrees(yaw), "pitch": math.degrees(pitch),
		        "face_px": face}
		return texture, pose

	def head_mask(self, canvas, points, face):
		"""Soft mask (0..1) of face, hair and headwear, and a mask of the face skin."""
		result = self.segmenter.segment(mp.Image(image_format=mp.ImageFormat.SRGB, data=canvas))
		confidence = [m.numpy_view().reshape(canvas.shape[:2]) for m in result.confidence_masks]
		head = confidence[HAIR] + confidence[FACE_SKIN] + confidence[ACCESSORIES]

		size = canvas.shape[0]
		yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
		oval = points[FACE_OVAL]
		middle = oval.mean(axis=0)
		# only keep things around the face: other people, hands below the chin, ...
		cx, cy = middle[0], middle[1] - 0.15 * face
		distance = np.sqrt(((xx - cx) / (0.75 * face)) ** 2 + ((yy - cy) / (0.80 * face)) ** 2)
		near = np.clip((1.15 - distance) / 0.3, 0, 1)
		# nothing below the chin (neck, shoulders, beards are hair and stay above it)
		chin = points[CHIN][1]
		near *= np.clip((chin + 0.12 * face - yy) / (0.08 * face), 0, 1)

		outline = np.zeros((size, size), np.uint8)
		cv2.fillConvexPoly(outline, cv2.convexHull(oval.astype(np.int32)), 1)
		keep = np.maximum(head * near, outline.astype(np.float32)) > 0.5

		# one piece: the part connected to the face
		count, labels = cv2.connectedComponents(keep.astype(np.uint8))
		label = labels[int(middle[1]), int(middle[0])]
		if label:
			keep = labels == label
		# fill holes (eyes or glasses classified as background)
		filled = keep.astype(np.uint8)
		contours, _ = cv2.findContours(filled, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
		cv2.drawContours(filled, contours, -1, 1, cv2.FILLED)
		alpha = cv2.GaussianBlur(filled.astype(np.float32), (0, 0), size / 300)
		skin = (confidence[FACE_SKIN] > 0.5) & (outline > 0)
		return alpha, skin


def turn(image, angle):
	"""The image turned by angle degrees (counter-clockwise), on a canvas that fits it."""
	h, w = image.shape[:2]
	if angle % 90 == 0:
		return np.ascontiguousarray(np.rot90(image, angle // 90))
	rotation = cv2.getRotationMatrix2D((w / 2, h / 2), angle, 1.0)
	cos, sin = abs(rotation[0, 0]), abs(rotation[0, 1])
	size = (int(w * cos + h * sin), int(w * sin + h * cos))
	rotation[:, 2] += (size[0] - w) / 2, (size[1] - h) / 2
	return cv2.warpAffine(image, rotation, size, flags=cv2.INTER_CUBIC, borderMode=cv2.BORDER_REPLICATE)


def eye_roll(points):
	"""Angle of the eye line against the horizontal, in radians."""
	d = points[RIGHT_EYE].mean(axis=0) - points[LEFT_EYE].mean(axis=0)
	return math.atan2(d[1], d[0])


def matrix_yaw_pitch(matrix):
	"""Yaw and pitch of the head from the face landmarker's transformation matrix."""
	r = matrix[:3, :3] / np.linalg.norm(matrix[:3, :3], axis=0)
	yaw = math.atan2(r[0, 2], r[2, 2])
	pitch = math.asin(max(-1.0, min(1.0, -r[1, 2])))
	return yaw, pitch


def apply(affine, points):
	return points @ affine[:, :2].T + affine[:, 2]


def even_lighting(image, skin, strength):
	"""Pull the average skin colour and brightness part of the way to a normal skin tone,
	against coloured party lights and dark rooms."""
	if strength <= 0 or skin.sum() < 50:
		return image
	pixels = image.astype(np.float32) / 255
	mean = pixels[skin].mean(axis=0)
	gain = (SKIN_TONE / np.maximum(mean, 0.05)) ** strength
	gain = np.clip(gain, 0.5, 2.5)
	return (np.clip(pixels * gain, 0, 1) * 255).astype(np.uint8)


def default_name(stem):
	match = re.match(r"person_0*(\d+)", stem)
	return f"Person {match.group(1)}" if match else stem


def file_name(name):
	slug = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode()
	slug = re.sub(r"[^a-z0-9]+", "_", slug.lower()).strip("_")
	return (slug or "face") + ".png"


def read_names(path):
	names = []
	for line in Path(path).read_text(encoding="utf-8").splitlines():
		line = line.split("#")[0].strip()
		if ";" in line:
			key, name = (part.strip() for part in line.split(";", 1))
			names.append((key, name))
	return names


def update_players(players_file, entries):
	"""Adds or updates "Name;face" lines, keeping everything else in players.txt."""
	lines = players_file.read_text(encoding="utf-8").splitlines() if players_file.exists() else []
	faces = {face: name for name, face in entries}
	kept = [line for line in lines
	        if not (";" in line and line.split(";", 1)[1].strip() in faces)
	        and line.split(";")[0].strip() not in {name for name, _ in entries}]
	kept += [f"{name};{face}" for name, face in entries]
	players_file.write_text("\n".join(kept) + "\n", encoding="utf-8")


def blob_preview(texture, color, scale=3):
	"""The texture on a blob, mapped like RenderManager::createSkinFrame (without shading)."""
	w, h = int(65 * scale), int(89 * scale)
	yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
	nx = (xx + 0.5 - w / 2) / (w / 2 * 0.9)
	ny = (yy + 0.5 - h / 2) / (h / 2 * 0.9)
	d = np.sqrt(nx ** 2 + ny ** 2)
	u = (0.5 + np.arcsin(np.clip(nx, -1, 1)) / math.pi) * TEXTURE_SIZE - 0.5
	v = (0.5 + np.arcsin(np.clip(ny, -1, 1)) / math.pi) * TEXTURE_SIZE - 0.5
	sample = cv2.remap(texture, u, v, cv2.INTER_LINEAR).astype(np.float32)
	t = np.clip((1 - d) / 0.12, 0, 1)
	fade = t * t * (3 - 2 * t) * sample[..., 3] / 255
	out = np.array(color, np.float32) * (1 - fade[..., None]) + sample[..., :3] * fade[..., None]
	body = (((xx + 0.5 - w / 2) / (w / 2)) ** 2 + ((yy + 0.5 - h / 2) / (h / 2)) ** 2) <= 1
	out[~body] = 255
	return out.astype(np.uint8)


def preview_sheet(rows, path):
	"""Photo, texture and blob for every player, four per row."""
	cell = 270
	colors = [(60, 120, 230), (230, 70, 70), (60, 180, 90), (240, 200, 50),
	          (150, 90, 210), (245, 140, 40), (40, 190, 200), (240, 110, 180)]
	tiles = []
	for i, (name, photo, texture) in enumerate(rows):
		tile = np.full((cell + 24, 3 * cell, 3), 255, np.uint8)
		thumb = photo.copy()
		s = cell / max(thumb.shape[:2])
		thumb = cv2.resize(thumb, (int(thumb.shape[1] * s), int(thumb.shape[0] * s)), interpolation=cv2.INTER_AREA)
		tile[:thumb.shape[0], :thumb.shape[1]] = thumb
		checker = ((np.indices((TEXTURE_SIZE, TEXTURE_SIZE)) // 16).sum(axis=0) % 2 * 40 + 200)[..., None]
		a = texture[..., 3:].astype(np.float32) / 255
		tex = (texture[..., :3] * a + checker * (1 - a)).astype(np.uint8)
		tile[7:7 + TEXTURE_SIZE, cell + 7:cell + 7 + TEXTURE_SIZE] = tex
		blob = blob_preview(texture, colors[i % len(colors)])
		tile[:blob.shape[0], 2 * cell + 37:2 * cell + 37 + blob.shape[1]] = blob
		cv2.putText(tile, name, (4, cell + 18), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 1, cv2.LINE_AA)
		tiles.append(tile)
	per_row = 4
	while len(tiles) % per_row:
		tiles.append(np.full_like(tiles[0], 255))
	sheet = np.vstack([np.hstack(tiles[i:i + per_row]) for i in range(0, len(tiles), per_row)])
	cv2.imwrite(str(path), cv2.cvtColor(sheet, cv2.COLOR_RGB2BGR), [cv2.IMWRITE_JPEG_QUALITY, 85])


def main():
	parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
	parser.add_argument("photos", nargs="+", type=Path, help="photos, or folders of photos")
	parser.add_argument("--out", type=Path, default=Path(__file__).resolve().parents[2] / "data" / "players",
	                    help="players folder of the game (default: data/players)")
	parser.add_argument("--names", type=Path, help='file with "photo;Name" lines')
	parser.add_argument("--preview", type=Path, help="write a contact sheet of the results (JPEG)")
	parser.add_argument("--color-fix", type=float, default=0.8,
	                    help="how far to correct the skin colour, 0 (off) to 1 (default 0.8)")
	parser.add_argument("--models", type=Path, default=Path.home() / ".cache" / "blobby-knockout")
	args = parser.parse_args()

	photos = []
	for path in args.photos:
		if path.is_dir():
			photos += sorted(p for p in path.iterdir() if p.suffix.lower() in (".jpg", ".jpeg", ".png", ".webp"))
		else:
			photos.append(path)
	names = read_names(args.names) if args.names else []

	download_models(args.models)
	cutter = FaceCutter(args.models)
	args.out.mkdir(parents=True, exist_ok=True)

	entries, rows, failed = [], [], []
	for photo_path in photos:
		image = cv2.imread(str(photo_path), cv2.IMREAD_COLOR)
		if image is None:
			failed.append(photo_path.name)
			continue
		image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
		texture, pose = cutter.cut(image, args.color_fix)
		if texture is None:
			print(f"{photo_path.name}: no face found", file=sys.stderr)
			failed.append(photo_path.name)
			continue
		name = next((n for key, n in names if photo_path.name.startswith(key)), default_name(photo_path.stem))
		face_file = file_name(name)
		cv2.imwrite(str(args.out / face_file), cv2.cvtColor(texture, cv2.COLOR_RGBA2BGRA))
		entries.append((name, face_file))
		rows.append((name, image, texture))
		print(f"{photo_path.name} -> {face_file}  {name:<16} roll {pose['roll']:6.1f}  "
		      f"yaw {pose['yaw']:6.1f}  pitch {pose['pitch']:6.1f}  face {pose['face_px']:5.0f}px")

	update_players(args.out / "players.txt", entries)
	print(f"{len(entries)} faces written to {args.out}, players.txt updated"
	      + (f"; failed: {', '.join(failed)}" if failed else ""))
	if args.preview and rows:
		preview_sheet(rows, args.preview)
		print(f"preview: {args.preview}")


if __name__ == "__main__":
	main()
