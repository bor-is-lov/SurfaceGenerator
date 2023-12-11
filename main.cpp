#include <iostream>
#include <vector>
#include <random>
#include <thread>
#define STB_IMAGE_WRITE_IMPLEMENTATION //stb_image_write.h requires this
#define __STDC_LIB_EXT1__ //it has to be defined for stb_image_write.h to work with C++ (may be a bug)
#include "stb_image_write.h"
//#define DEBUG
using namespace std;

struct Color{
	unsigned char red = 0;
	unsigned char green = 0;
	unsigned char blue = 0;
};

int main(){
	srand(time(NULL));
	size_t height, width;
	cout << "Enter the image resolution (each pixel is a block):\nWidth: ";
	cin >> width;
	cout << "Height: ";
	cin >> height;
#ifdef DEBUG
	auto start = chrono::high_resolution_clock::now();
#endif // DEBUG
	vector<vector<double>> land(height, vector<double>(width));
	vector<vector<double>> tempLand(height, vector<double>(width));
	for (auto& row : land)
		for (auto& block : row)
			block = (rand() % 201) / 100.0 - 1;//filling land with random values from -1 to 1

	//smoothing
	auto smooth = [&land, &tempLand, &height, &width](int x1, int x2){
		for (size_t y = 0; y < height; ++y)
			for (size_t x = x1; x < x2; ++x){
				double sum = land[y][x];
				int amount = 1;
				if (x != 0){
					++amount;
					sum += land[y][x - 1];
				}
				if (y != 0){
					++amount;
					sum += land[y - 1][x];
				}
				if (x != width - 1){
					++amount;
					sum += land[y][x + 1];
				}
				if (y != height - 1){
					++amount;
					sum += land[y + 1][x];
				}
				if (x != 0 && y != 0){
					++amount;
					sum += land[y - 1][x - 1];
				}
				if (x != 0 && y != height - 1){
					++amount;
					sum += land[y + 1][x - 1];
				}
				if (x != width - 1 && y != height - 1){
					++amount;
					sum += land[y + 1][x + 1];
				}
				if (x != width - 1 && y != 0){
					++amount;
					sum += land[y - 1][x + 1];
				}
				tempLand[y][x] = sum / amount;
			}
	};
	int thSize = thread::hardware_concurrency() - 1;
	if (thSize < 1) return -1;
	thread* threads = new thread[thSize];
	for (size_t i = 0; i < 100; i++){//more times = huger areas, but lower performance
		for (size_t j = 0; j < thSize; ++j){
			int x1 = width / thSize * j;
			int x2 = (j < thSize - 1) ? width / thSize * (j + 1) : width;
			threads[j] = thread(smooth, x1, x2);
		}
		for (size_t j = 0; j < thSize; ++j)
			threads[j].join();
		for (size_t y = 0; y < height; ++y)
			for (size_t x = 0; x < width; ++x)
				land[y][x] = tempLand[y][x];
	}
	delete[] threads;
	for (auto& row : tempLand)
		row.~vector();
	tempLand.~vector();

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

	//setting colors according to the land height
	vector<vector<Color>> img(height, vector<Color>(width));
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
		cout << "Image created successfully\n";
	else
		cout << "Error during creating image\n";
	delete[] imgData;
#ifdef DEBUG
	auto end = chrono::high_resolution_clock::now();
	chrono::duration<float> duration = end - start;
	cout << "Duration: " << duration.count() << "\a\n";
#endif // DEBUG
	return 0;
}