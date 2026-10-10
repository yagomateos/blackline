"""Render rápido (Workbench) de un FBX para revisar assets sin abrir Blender.

Uso:
  blender -b -P preview_render.py -- <entrada.fbx> <salida.png> [azimut_grados]
"""
import math
import os
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
src, out = argv[0], argv[1]
az = math.radians(float(argv[2])) if len(argv) > 2 else math.radians(35)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src)
objs = [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.name.startswith('UCX_')]
for o in bpy.context.scene.objects:
    if o.name.startswith('UCX_'):
        o.hide_render = True   # colisiones: no se ven

mn = Vector((1e9, 1e9, 1e9))
mx = Vector((-1e9, -1e9, -1e9))
for o in objs:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c)
        mn = Vector(map(min, mn, w))
        mx = Vector(map(max, mx, w))
center = (mn + mx) / 2
radius = (mx - mn).length / 2

cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 35
cam = bpy.data.objects.new("Cam", cam_data)
bpy.context.scene.collection.objects.link(cam)
dist = radius * 2.6
cam.location = center + Vector((math.sin(az) * dist, -math.cos(az) * dist, radius * 0.9))
cam.rotation_euler = (center - cam.location).to_track_quat('-Z', 'Y').to_euler()
bpy.context.scene.camera = cam

scene = bpy.context.scene
scene.render.engine = 'BLENDER_WORKBENCH'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'MATERIAL'
scene.display.shading.show_cavity = True
scene.display.shading.show_shadows = True
scene.render.resolution_x = 1280
scene.render.resolution_y = 960
scene.render.filepath = os.path.abspath(out)
bpy.ops.render.render(write_still=True)
print(f"[preview_render] {out}")
