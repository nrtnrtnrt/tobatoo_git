import numpy as np
import matplotlib.pyplot as plt

img_names = [f"mask_result{i}" for i in range(1, 40)]
mask_names = [f"padding_result{i}" for i in range(1, 40)]

for img_name, mask_name in zip(img_names,mask_names):
    with open(img_name, "rb") as f:
        data = f.read()
    with open(mask_name, "rb") as f:
        data_mask = f.read()
    img = np.frombuffer(data, dtype=np.uint8).reshape((256, 256, -1))
    mask = np.frombuffer(data_mask, dtype=np.uint8).reshape((256, 256, -1))
    fig, axs = plt.subplots(2, 1)
    axs[0].imshow(img)
    axs[1].imshow(mask)
    plt.title(f"img : {img_name}")
    plt.show()
