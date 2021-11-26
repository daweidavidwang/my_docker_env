import numpy as np
import cv2
from matplotlib import pyplot as plt 
from sklearn.cluster import KMeans




img = cv2.imread('gazebo3.pgm')
kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(19, 19))
img_dilated = cv2.erode(img, kernel)
plt.imshow(img_dilated)
plt.show()

gray = cv2.cvtColor(img_dilated,cv2.COLOR_BGR2GRAY)
print(np.shape(gray))

img_high = np.shape(gray)[0]
img_width = np.shape(gray)[1]

# pixel 941 x 788
def pixel_to_world(pixel_x, pixel_y):
    world_x = (pixel_x - img_width/2.0)*0.05
    world_y = -(pixel_y - img_high/2.0)*0.05
    return world_x, world_y

format = '<Agent p_x=\"%.3f\" p_y=\"%.3f\" />\n'
b = format%(0.2, 0.3)
print(b)

file_name  = open('agent_loc.txt', 'w')
num_agent = 100
valid_num = 0
while valid_num < num_agent:
    sead_x = np.random.random() * img_width
    sead_y = np.random.random() * img_high
    if(gray[int(sead_y),int(sead_x)] != 254):
        continue
    valid_num += 1
    world_sead_x, world_sead_y = pixel_to_world(sead_x, sead_y)
    print(world_sead_x)
    file_name.write(format%(world_sead_x, world_sead_y))
    


