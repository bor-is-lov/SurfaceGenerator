#include <iostream>
#include <vector>
#include <random>
#include <thread>
#define STB_IMAGE_WRITE_IMPLEMENTATION //stb_image_write.h requires this
#define __STDC_LIB_EXT1__ //it has to be defined for stb_image_write.h to work with C++ (may be a bug)
#include "stb_image_write.h"
#define DEBUG 1

struct Color{
	unsigned char red = 0;
	unsigned char green = 0;
	unsigned char blue = 0;
};

int main(){
	srand(1);
	size_t height, width;
	std::cout << "Enter the image resolution (each pixel is a block):\nWidth: ";
	std::cin >> width;
	std::cout << "Height: ";
	std::cin >> height;
#if DEBUG == 1
	auto start = std::chrono::high_resolution_clock::now();
#endif // DEBUG
	std::vector<std::vector<double>>
		src(height, std::vector<double>(width)),
		land(height, std::vector<double>(width)),
		land2(height, std::vector<double>(width));

	for (size_t y = 0; y < height; ++y)
		for (size_t x = 0; x < width; ++x)
			src[x][y] = (rand() % 201) / 100.0 - 1;//filling land with random values from -1 to 1

	//smoothing
	auto smooth = [&](std::vector<std::vector<double>>* land, int x1, int x2, int radius){
		for (int y = 0; y < height; ++y)
			for (int x = x1; x < x2; ++x)
			{
				double sum = 0;
				int amount = 0;
				for (int yr = y - radius; yr < y + radius; yr++)
					for (int xr = x - radius; xr < x + radius; xr++)
					{
						if (xr < 0 || yr < 0 || xr >= width || yr >= height)
							continue;
						double influence = 1 - sqrt((x - xr) * (x - xr) + (y - yr) * (y - yr)) / radius;
						if (influence > 0.0)
							sum += src[xr][yr] * influence;
						amount++;
					}
				(*land)[x][y] = sum / amount;
			}
	};
	int thSize = std::thread::hardware_concurrency() - 1;
	if (thSize < 1) return 1;
	std::thread* threads = new std::thread[thSize];
	{
		for (size_t j = 0; j < thSize; ++j){
			int x1 = width / thSize * j;
			int x2 = (j < thSize - 1) ? width / thSize * (j + 1) : width;
			threads[j] = std::thread(smooth, &land, x1, x2, 100);
		}
		for (size_t j = 0; j < thSize; ++j)
			threads[j].join();
	}
	{
		for (size_t j = 0; j < thSize; ++j){
			int x1 = width / thSize * j;
			int x2 = (j < thSize - 1) ? width / thSize * (j + 1) : width;
			threads[j] = std::thread(smooth, &land2, x1, x2, 10);
		}
		for (size_t j = 0; j < thSize; ++j)
			threads[j].join();
	}
	delete[] threads;
	for (auto& row : src)
		row.~vector();
	src.~vector();

	//scaling back to [-1; 1]
	double maxEl = -1;
	for (auto& row : land)
		for (auto& block : row)
			if (maxEl < block || maxEl < abs(block))
				maxEl = abs(block);
	double factor = 1 / maxEl;
	for (auto& row : land)
		for (auto& block : row)
			block *= factor;

	maxEl = -1;
	for (auto& row : land2)
		for (auto& block : row)
			if (maxEl < block || maxEl < abs(block))
				maxEl = abs(block);
	factor = 1 / maxEl;
	for (auto& row : land2)
		for (auto& block : row)
			block *= factor;

	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
			land[x][y] = land[x][y] + land2[x][y]/10;

	//setting colors according to the land height
	std::vector<std::vector<Color>> img(height, std::vector<Color>(width));
	for (size_t y = 0; y < height; ++y)
		for (size_t x = 0; x < width; ++x)
			if (land[y][x] < 0)
				img[y][x].blue = 255 + land[y][x] * 200;
			else if (land[y][x] > 0.1){
				img[y][x].green = 255;
				img[y][x].blue = img[y][x].red = land[y][x] * 255;
			}
			else{
				img[y][x].blue = land[y][x] * 255;
				img[y][x].green = img[y][x].red = 200;
			}

	//converting vector to data that can be written with stb_image_write.h
	unsigned char* imgData = new unsigned char[(height * width) * 3];
	for (size_t y = 0; y < height; y++)
		for (size_t x = 0; x < width; x++){
			imgData[(width * y + x) * 3] = img[y][x].red;
			imgData[(width * y + x) * 3 + 1] = img[y][x].green;
			imgData[(width * y + x) * 3 + 2] = img[y][x].blue;
		}

	//writing data to image file
	if (stbi_write_png("image.png", width, height, 3, imgData, 0))
		std::cout << "Image created successfully\n";
	else
		std::cout << "Error during creating image\n";
	delete[] imgData;
#if DEBUG == 1
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> duration = end - start;
	std::cout << "Duration: " << duration.count() << "\a\n";
#endif // DEBUG
	return 0;
}