import os, re, sys
from PIL import Image, ImageDraw, ImageFont

srn = sys.argv[1]
rows = re.findall(r"\| \*\*(S\d\d)\*\* \|.*?\| (.*?) \|", open("README.md").read())
font = ImageFont.load_default(size=22)
shots = os.path.expanduser("~/shots")
pages = []
for sid, caption in rows:
    img = Image.open(f"{shots}/{sid}.png").convert("RGB")
    img.thumbnail((1600, 1600))
    canvas = Image.new("RGB", (img.width, img.height + 90), "white")
    canvas.paste(img, (0, 0))
    ImageDraw.Draw(canvas).text((10, img.height + 15), caption, fill="black", font=font)
    pages.append(canvas)
pages[0].save(f"{srn}.pdf", save_all=True, append_images=pages[1:])
print("Created", f"{srn}.pdf", "with", len(pages), "pages")
