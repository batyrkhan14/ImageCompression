#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <ctime>
#include <unordered_map>

using namespace cv;
using namespace std;
string PATH = "/Users/nursultan/Desktop/NU/Spring 2017/unix/images/";

int max(int a, int b){
	if (a > b) return a;
	return b;
}
void push_back(vector<bool> &v, int x) {
	for (int i = 0; i < 8; i++) {
		v.push_back((x >> (7 - i))&1);
	}
}

void write(vector<bool> &v, int bits, int x){
	for (int i = 0; i < bits; i++){
		v.push_back((x >> (bits-1-i))&1);
	}
}

double  compressLZW(string inputFile, string outputFile) {
	Mat mat = imread(inputFile, 0);
	int width = mat.cols;
	int height = mat.rows;
	vector<uchar> image;
	image.assign(mat.datastart, mat.dataend);
	unordered_map<vector<bool>, int> dict;
	vector<int> encoding;
	int cntr = 1;
	for (int i = 0; i < 256; i++) {
		vector<bool> temp;
		push_back(temp, i);
		dict[temp] = cntr++;
	}
	int i = 0;
	while (i < image.size()) {
		vector<bool> temp;
		push_back(temp, image[i]);
		i++;
		while (true) {
			if (i >= image.size()) {
				encoding.push_back(dict[temp]);
				break;
			}
			int prev = dict[temp];
			push_back(temp, image[i]);
			if (dict.find(temp) == dict.end()) {
				encoding.push_back(prev);
				dict[temp] = cntr++;
				break;
			}
			i++;
		}
	}

	int mx = 0;
	for (int i = 0; i < encoding.size(); i++) {
		mx = max(mx, encoding[i]);
	}
	int sz = 0;
	while (mx > 0) {
		mx /= 2;
		sz++;
	}
	vector<bool> toWrite;
	write(toWrite, 8, sz);
	write(toWrite, 16, width);
	write(toWrite, 16, height);
	for (int i = 0; i < encoding.size(); i++) {
		write(toWrite, sz, encoding[i]);
	}
	while (toWrite.size() % 8 != 0) {
		toWrite.push_back(false);
	}
	vector<char> final;
	for (int i = 0; i < toWrite.size(); i += 8) {
		uchar x = 0;
		for (int j = i; j < i + 8; j++) {
			x = x * 2 + toWrite[j];
		}
		final.push_back((char)x);
	}
	FILE* pFile;
	pFile = fopen(outputFile.c_str(), "wb");
	fwrite(&final[0], sizeof(vector<char>::value_type), final.size(), pFile);
	fclose(pFile);
	return 1.0*height*width/final.size();
}
void decompressLZW(string inputFile, string outputFile) {
	ifstream input(inputFile, ios::binary);
	vector<char> buffer((
		istreambuf_iterator<char>(input)),
		(istreambuf_iterator<char>()));
	int sz = ((uchar)buffer[0]);
	int width = ((int)((uchar)buffer[1]))*256+((int)((uchar)buffer[2]));
	int height = ((int)((uchar)buffer[3]))*256+((int)((uchar)buffer[4]));
	vector<int> encoding;
	int curr = 0, bits = 0;
	for (int i = 5; i < buffer.size(); i++) {
		for (int j = 0; j < 8; j++) {
			curr = curr * 2 + (int)((((uchar)buffer[i]) >> (7 - j))&1);
			bits++;
			if (bits == sz) {
				encoding.push_back(curr);
				curr = 0;
				bits = 0;
			}
		}
	}
	
	unordered_map<int, vector<uchar> > dict;
	vector<uchar> decoding;
	int cntr = 1;
	for (int i = 0; i < 256; i++) {
		vector<uchar> temp;
		temp.push_back((uchar)i);
		dict[cntr++] = temp;
	}
	
	int pw, cw;
	
	for (int i = 0; i < encoding.size(); i++) {
		cw = encoding[i];
		if (dict.find(cw) != dict.end()) {
			decoding.insert(decoding.end(), dict[cw].begin(), dict[cw].end());
			if (i > 0) {
				vector<uchar> temp;
				temp.insert(temp.end(), dict[pw].begin(), dict[pw].end());
				temp.push_back(dict[cw][0]);
				dict[cntr++] = temp;
			}
		}
		else {
			vector<uchar> temp;
			temp.insert(temp.end(), dict[pw].begin(), dict[pw].end());
			temp.push_back(dict[pw][0]);
			decoding.insert(decoding.end(), temp.begin(), temp.end());
			dict[cntr++] = temp;
		}
		pw = cw;
	}
	Mat out(width, height, CV_8UC1, Scalar(255));
	for (int i = 0; i < height; i++){
		for (int j = 0; j < width; j++){
			out.at<uchar>(i, j) = decoding[i*width+j];
		}
	}
	imwrite(outputFile, out);
}

void testSample(){

	for (int i = 1; i <= 16; i++) {

		clock_t begin, end;
		double elapsed_secs;

		begin = clock();
		double ratio = compressLZW(PATH+to_string(i)+".bmp", PATH+"encoded/"+to_string(i)+".pku");
		end = clock();
		elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

		cout << "Image " + to_string(i) + ": " << endl;
		cout << "Compression time: " << elapsed_secs << " seconds" << endl;
		cout << "Compression ratio: " << ratio << endl;

		begin = clock();
		decompressLZW(PATH+"encoded/"+to_string(i)+".pku",PATH+"decoded/"+to_string(i)+".bmp");
		end = clock();
		elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

		cout << "Decompression time: " << elapsed_secs << " seconds" << endl << endl;
	}

}
int main(int argc, char* argv[]) {
	if (argc == 4){
		char* command = argv[1];
		char* inputFile = argv[2];
		char* outputFile = argv[3];
		if (strcmp(command, "compress") == 0){
			compressLZW(inputFile, outputFile);
		}
		else if (strcmp(command, "decompress") == 0){
			decompressLZW(inputFile, outputFile);
		}
	}
	//To run compression and decompression for given sample images
	//testSample();
	return 0;
}



