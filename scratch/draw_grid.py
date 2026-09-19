from PIL import Image, ImageDraw
import numpy as np

# Load frames 26 (before turn) and 32 (after turn) and 40 (end of command)
for f_name in ['frame_026.jpg', 'frame_030.jpg', 'frame_032.jpg', 'frame_035.jpg', 'frame_040.jpg']:
    img = Image.open(f'scratch/video_frames/{f_name}')
    w, h = img.size
    
    # Let's crop around the robot or draw horizontal reference grid lines
    draw = ImageDraw.Draw(img)
    # Draw horizontal grid lines across the image to compare tile lines
    for y in range(0, h, 100):
        draw.line([(0, y), (w, y)], fill='yellow', width=1)
    for x in range(0, w, 100):
        draw.line([(x, 0), (x, h)], fill='cyan', width=1)
        
    out_path = f'scratch/video_frames/grid_{f_name}'
    img.save(out_path)
    print(f"Saved {out_path}")
