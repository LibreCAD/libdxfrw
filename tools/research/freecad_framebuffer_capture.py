"""Capture a generated mesh and line with controlled visibility transitions."""
from pathlib import Path


NAMES = ("pixel-visible.png", "pixel-mesh-hidden.png",
         "pixel-all-hidden.png", "pixel-restored.png")


def capture(view, mesh, line, directory, process_events):
    root = Path(directory)
    if not root.is_dir() or any((root / name).exists() for name in NAMES):
        raise ValueError("capture directory is missing or already contains images")
    if not mesh.Visibility or not line.Visibility:
        raise ValueError("mesh and line must initially be visible")
    view.viewTop()
    view.fitAll()
    process_events()
    try:
        for name, mesh_visible, line_visible in (
                (NAMES[0], True, True), (NAMES[1], False, True),
                (NAMES[2], False, False), (NAMES[3], True, True)):
            mesh.Visibility = mesh_visible
            line.Visibility = line_visible
            process_events()
            if bool(mesh.Visibility) != mesh_visible or bool(line.Visibility) != line_visible:
                raise RuntimeError(f"visibility changed before {name}: "
                                   f"mesh={bool(mesh.Visibility)}, line={bool(line.Visibility)}")
            view.saveImage(str(root / name), 320, 240, "White")
    finally:
        mesh.Visibility = True
        line.Visibility = True
        process_events()
    return list(NAMES)
