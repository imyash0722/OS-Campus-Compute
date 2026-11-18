import os
import re
import sys
from PIL import Image, ImageDraw, ImageFont

def get_font():
    font_candidates = [
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
    ]
    for fp in font_candidates:
        if os.path.exists(fp):
            try:
                return ImageFont.truetype(fp, 18)
            except Exception:
                pass
    return ImageFont.load_default(size=18)

def wrap_text(text, font, max_width, draw):
    words = text.split()
    lines = []
    cur = []
    for w in words:
        test = " ".join(cur + [w])
        bbox = draw.textbbox((0, 0), test, font=font)
        if (bbox[2] - bbox[0]) <= max_width:
            cur.append(w)
        else:
            if cur:
                lines.append(" ".join(cur))
                cur = [w]
            else:
                lines.append(w)
                cur = []
    if cur:
        lines.append(" ".join(cur))
    return lines

def main():
    srn = sys.argv[1] if len(sys.argv) > 1 else "PES2UG25CS606"
    
    shots_dir = None
    for candidate in ["./shots", os.path.expanduser("~/shots"), "/mnt/shared/sem3/os/OS-Campus-Compute/shots"]:
        if os.path.isdir(candidate):
            shots_dir = candidate
            break
    
    if not shots_dir:
        print("Error: Screenshots directory not found (checked ./shots and ~/shots)")
        sys.exit(1)

    readme_path = "README.md"
    if not os.path.exists(readme_path):
        readme_path = os.path.join(os.path.dirname(__file__), "README.md")
    
    if not os.path.exists(readme_path):
        print(f"Error: {readme_path} not found")
        sys.exit(1)

    rows = []
    for line in open(readme_path, encoding="utf-8").read().splitlines():
        if not line.strip().startswith("|"):
            continue
        cols = [c.strip() for c in re.split(r"(?<!\\)\|", line.strip()) if c.strip()]
        if len(cols) >= 3 and re.match(r"\*{0,2}S\d\d\*{0,2}", cols[0]):
            sid = re.sub(r"[^A-Za-z0-9]", "", cols[0])
            caption = cols[-1]
            rows.append((sid, caption))

    font = get_font()
    dummy = Image.new("RGB", (1, 1))
    draw_dummy = ImageDraw.Draw(dummy)
    bbox = draw_dummy.textbbox((0, 0), "Ay", font=font)
    line_h = int((bbox[3] - bbox[1]) * 1.4)

    pages = []
    for sid, caption in rows:
        img_path = os.path.join(shots_dir, f"{sid}.png")
        if not os.path.exists(img_path):
            print(f"Warning: {img_path} not found, skipping.")
            continue

        img = Image.open(img_path).convert("RGB")

        # Preserve aspect ratio while ensuring max width does not exceed 1600
        if img.width > 1600:
            new_h = int(img.height * (1600.0 / img.width))
            img = img.resize((1600, new_h), Image.Resampling.LANCZOS)

        pad_x = 20
        pad_y = 15
        max_text_width = img.width - (2 * pad_x)
        wrapped_lines = wrap_text(caption, font, max_text_width, draw_dummy)
        caption_height = max(70, pad_y * 2 + len(wrapped_lines) * line_h)

        canvas = Image.new("RGB", (img.width, img.height + caption_height), "white")
        canvas.paste(img, (0, 0))

        draw = ImageDraw.Draw(canvas)
        y = img.height + pad_y
        for line in wrapped_lines:
            draw.text((pad_x, y), line, fill="black", font=font)
            y += line_h

        pages.append(canvas)

    if not pages:
        print("Error: No pages were created.")
        sys.exit(1)

    out_pdf = f"{srn}.pdf"
    pages[0].save(out_pdf, save_all=True, append_images=pages[1:], resolution=100.0)
    print(f"Created {out_pdf} with {len(pages)} pages")

if __name__ == "__main__":
    main()
