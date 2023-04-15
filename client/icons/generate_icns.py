from PIL import Image
import os, sys, shutil

img = Image.open(sys.argv[1])
if img.size != (1024,1024):
    print "Input image must be 1024x1024. Provided image is %dx%d" % img.size

os.mkdir("vcmi.iconset")
for i in [16, 32, 128, 256, 512]:
    resized = img.resize((i, i), Image.ANTIALIAS)
    resized.save("vcmi.iconset/icon_%dx%d.png" % (i, i))

    resized2x = img.resize((2*i, 2*i), Image.ANTIALIAS)
    resized2x.save("vcmi.iconset/icon_%dx%d@2x.png" % (i, i))

os.system("iconutil -c icns vcmi.iconset")
shutil.rmtree("vcmi.iconset")
