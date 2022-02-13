// main.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SOM.h"

#pragma warning (disable : 4996)

#define SOM_SIDE 7
#define SOM_STEP 1
#define LR_SIDE 3  
#define HR_SIDE 6
#define SOM1_SIGMA 0.5
#define SOM2_SIGMA 0.5
#define NUM_LAYERS 1

#define CLK_TO_SEC(_clk_)  (((double) (_clk_)) / ((double) CLOCKS_PER_SEC))

#if NUM_LAYERS == 1
//		    //
// 1 LAYER //
//		  //

void saveSOM(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, const char* path)
{
	FILE *file = NULL;
	char tmpPath[2048];
	sprintf(tmpPath, "%s.tmp", path);
	fopen_s(&file, tmpPath, "wb");
	if (file)
	{
		som.save(file);
		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
	remove(path);
	rename(tmpPath, path);

}

void loadSOM(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, const char* path)
{
	FILE *file = NULL;
	fopen_s(&file, path, "rb");
	if (file)
	{
		som.load(file);
		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}

void testImage(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, const Image &lrImg, const char* path)
{
	// Image to save the SOM version of the LR-image
	Image tempLRImg(lrImg.getWidth(), lrImg.getHeigth());
	Image scoresLo(lrImg.getWidth(), lrImg.getHeigth());
	// Dimensions of the LRImage
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;

	// Full destination HR-image and scores
	const unsigned int heightHR = (maxHeightLR + LR_SIDE) * 2;
	const unsigned int widthHR = (maxWidthLR + LR_SIDE) * 2;
	Image hrImg(widthHR, heightHR);
	Image scoresHi(widthHR, heightHR);
	Patch<LR_SIDE> lrPatch = Patch<LR_SIDE>();
	Patch<HR_SIDE> scores = Patch<HR_SIDE>();
	Patch<HR_SIDE> hrPatch = Patch<HR_SIDE>();

	// Processing
	for (unsigned int y = 0; y < maxHeightLR; ++y)
		for (unsigned int x = 0; x < maxWidthLR; ++x)
		{
			// Load current patch
			lrPatch.readPatch(lrImg, y, x);

			// Tests patch
			som.test(lrPatch, hrPatch, &scores);
			hrPatch.addPatch(hrImg, 2 * y, 2 * x);
			scores.addPatch(scoresHi, 2 * y, 2 * x);
		}
	for (unsigned int y = 0; y < heightHR; ++y)
		for (unsigned int x = 0; x < widthHR; ++x)
			hrImg.setValue(hrImg.getImage()[y][x] / scoresHi.getImage()[y][x], y, x);
	hrImg.exportImagePGM(path);
}

void trainImage(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, const Image &lrImg, const Image &hrImg)
{

	// Checks if the high-resoluton image is 2 times the low-resolution image
	if (2 * lrImg.getHeigth() != hrImg.getHeigth() || 2 * lrImg.getWidth() != hrImg.getWidth())
	{
		printf("ERROR: Low resolution image dimensions doesnt match high resolution image");
		return;
	}

	// Get max dimensions for the low-resolution Image
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	printf("LRwidth:%u(%u) LRheight:%u(%u)\n", maxWidthLR + LR_SIDE, lrImg.getWidth(), maxHeightLR + LR_SIDE, lrImg.getHeigth());

	// New cell to load the Patches to train
	Cell<LR_SIDE, HR_SIDE> cell = Cell<LR_SIDE, HR_SIDE>(1);

	for (unsigned int y = 0; y < maxHeightLR; y += SOM_STEP)
	{
		for (unsigned int x = 0; x < maxWidthLR; x += SOM_STEP)
		{
			// Load current LR-patch
			cell.getLowPatch().readPatch(lrImg, y, x);

			// Load current HR-patch
			cell.getHighPatch().readPatch(hrImg, 2 * y, 2 * x);

			// Train current patch
			som.train(cell);
		}
	}
}

void testSOM(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, const char* pathLR, const char* outputPath)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);

	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			Image lrImg(tmpLR);
			sprintf(tmpHR, "%s/%s", outputPath, pgm_file.name);
			printf("\nTesting image %d:\n  * %s\n  *output:  %s\n", count, tmpLR, tmpHR);

			// Tests full image
			const clock_t  clkIni = clock();
			testImage(som, lrImg, tmpHR);
			printf("  * Tested image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

void trainSOM(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, const char* pathLR, const char* pathHR, const char* pathSOM, const char* pathEXP)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);
	printf("Searching .pgm in %s\n", pathLR);
	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			sprintf(tmpHR, "%s/%s", pathHR, pgm_file.name);
			printf("\nTraining image %d:\n  * %s\n  * %s\n", count, tmpLR, tmpHR);

			// Loads both low-resolution and high-resolution images
			Image lrImg(tmpLR);
			Image hrImg(tmpHR);

			// Train images
			const clock_t  clkIni = clock();
			trainImage(som, lrImg, hrImg);
			printf("  * Trained image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			//Save SOM after each image
			saveSOM(som, pathSOM);
			char pathEXPtmp[2048];
			//Export SOM images if path not NULL 
			if (pathEXP != NULL)
			{
				printf("Saving som: %s(-LR/-HR)\n", pathEXP);
				som.exportSOMImage(pathEXPtmp, 127.5, 96);	
			}
			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

int main(int argc, char*argv[])
{
	const char *pathSOM = NULL;
	const char *pathLR = NULL;
	const char *pathHR = NULL;
	const char *pathEXP = NULL;
	bool train = false;
	bool load = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strstr(argv[i], "-sv:") == argv[i])
		{
			pathSOM = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-ld") == 0)
		{
			load = true;
		}
		else if (strstr(argv[i], "-e:") == argv[i])
		{
			pathEXP = argv[i] + 3;
		}
		else if (strstr(argv[i], "-lr:") == argv[i])
		{
			pathLR = argv[i] + 4;
		}
		else if (strstr(argv[i], "-hr:") == argv[i])
		{
			pathHR = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-test") == 0)
		{
			train = false;
			load = true;
		}
		else if (strcmp(argv[i], "-learn") == 0)
		{
			train = true;
		}
		else
		{
			printf("Unknown argument: %s\n", argv[i]);
			return -1;
		}
	}
	// CREATE SOM
	printf("-- Superresolution SOM --\n");
	SOM<SOM_SIDE, LR_SIDE, HR_SIDE> som(SOM_STEP, SOM1_SIGMA);
	printf("SOM created\n");

	// LOAD SOM
	if (load && (pathSOM != NULL))
	{
		loadSOM(som, pathSOM);
		printf("SOMs loaded\n");
	}

	// START //

	if (train)	// TRAINING
	{
		if (pathSOM == NULL)
		{
			printf("There is no path to save the SOM\n");
			return -1;
		}
		printf("Training model...\n");
		trainSOM(som, pathLR, pathHR, pathSOM, pathEXP);
	}
	else		// TESTING
	{
		printf("Testing model...\n");
		testSOM(som, pathLR, pathHR);
	}
	printf("End.\n");
}
#endif

  //		  //
 // 2 LAYERS //
//		    //

#if NUM_LAYERS == 2

void saveSOM(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM[SOM_SIDE][SOM_SIDE], const char* path)
{
	FILE *file = NULL;
	char tmpPath[2048];
	sprintf(tmpPath, "%s.tmp", path);
	fopen_s(&file, tmpPath, "wb");
	if (file)
	{
		som.save(file);
		fflush(file);
	
		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{				
					arraySOM[y][x]->save(file);
					fflush(file);		
			}
		fclose(file);	
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
	remove(path);
	rename(tmpPath,path);

}

void loadSOM(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE], const char* path)
{
	FILE *file = NULL;
	fopen_s(&file, path, "rb");
	if (file)
	{
		som.load(file);
		for (unsigned int y = 0; y < SOM_SIDE; ++y) 
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->load(file);
			}
		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}

void testImage(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM[SOM_SIDE][SOM_SIDE], const Image &lrImg, const char* path)
{
	// Image to save the SOM version of the LR-image
	Image tempLRImg(lrImg.getWidth(), lrImg.getHeigth());
	Image scoresLo(lrImg.getWidth(), lrImg.getHeigth());
	// Dimensions of the LRImage
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;

	// Full destination HR-image and scores
	const unsigned int heightHR = (maxHeightLR + LR_SIDE) * 2;
	const unsigned int widthHR = (maxWidthLR + LR_SIDE) * 2;
	Image hrImg(widthHR, heightHR);
	Image scoresHi(widthHR, heightHR);
	Patch<LR_SIDE> lrPatch = Patch<LR_SIDE>();
	Patch<HR_SIDE> scores = Patch<HR_SIDE>();
	Patch<HR_SIDE> hrPatch = Patch<HR_SIDE>();

	// Processing
	for (unsigned int y = 0; y < maxHeightLR; ++y)
		for (unsigned int x = 0; x < maxWidthLR; ++x)
		{
			// Load current patch
			lrPatch.readPatch(lrImg, y, x);

			// Tests patch
			unsigned int best_i, best_j;
			som.getBest(best_i,best_j,lrPatch);
			arraySOM[best_i][best_j]->test(lrPatch, hrPatch, &scores);
			hrPatch.addPatch(hrImg, 2*y, 2*x);
			scores.addPatch(scoresHi, 2*y, 2*x);
		}
	for (unsigned int y = 0; y < heightHR; ++y)
		for (unsigned int x = 0; x < widthHR; ++x)
			hrImg.setValue(hrImg.getImage()[y][x] / scoresHi.getImage()[y][x], y, x);
	hrImg.exportImagePGM(path);		
}

void trainImage(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE], const Image &lrImg, const Image &hrImg)
{

	// Checks if the high-resoluton image is 2 times the low-resolution image
	if (2 * lrImg.getHeigth() != hrImg.getHeigth() || 2 * lrImg.getWidth() != hrImg.getWidth())
	{
		printf("ERROR: Low resolution image dimensions doesnt match high resolution image");
		return;
	}

	// Get max dimensions for the low-resolution Image
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	printf("LRwidth:%u(%u) LRheight:%u(%u)\n", maxWidthLR + LR_SIDE, lrImg.getWidth(), maxHeightLR + LR_SIDE, lrImg.getHeigth());

	// New cell to load the Patches to train
	Cell<LR_SIDE, HR_SIDE> cell = Cell<LR_SIDE, HR_SIDE>(1);

	for (unsigned int y = 0; y < maxHeightLR; y += SOM_STEP)
	{
		for (unsigned int x = 0; x < maxWidthLR; x += SOM_STEP)
		{
			// Load current LR-patch
			cell.getLowPatch().readPatch(lrImg, y, x);

			// Load current HR-patch
			cell.getHighPatch().readPatch(hrImg, 2 * y, 2 * x);

			// Train current patch
			unsigned int best_i, best_j;
			const double score = som.train(cell, &best_i, &best_j);
			arraySOM[best_i][best_j]->train(cell);
		}
	}
}

void testSOM(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM[SOM_SIDE][SOM_SIDE], const char* pathLR, const char* outputPath)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);

	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			Image lrImg(tmpLR);
			sprintf(tmpHR, "%s/%s", outputPath, pgm_file.name);
			printf("\nTesting image %d:\n  * %s\n  *output:  %s\n", count, tmpLR, tmpHR);

			// Tests full image
			const clock_t  clkIni = clock();
			testImage(som, arraySOM, lrImg, tmpHR);
			printf("  * Tested image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

void trainSOM(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE], const char* pathLR, const char* pathHR, const char* pathSOM, const char* pathEXP)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);
	printf("Searching .pgm in %s\n", pathLR);
	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			sprintf(tmpHR, "%s/%s", pathHR, pgm_file.name);
			printf("\nTraining image %d:\n  * %s\n  * %s\n", count, tmpLR, tmpHR);

			// Loads both low-resolution and high-resolution images
			Image lrImg(tmpLR);
			Image hrImg(tmpHR);

			// Train images
			const clock_t  clkIni = clock();
			trainImage(som, arraySOM, lrImg, hrImg);
			printf("  * Trained image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			//Save SOM after each image
			saveSOM(som, arraySOM, pathSOM);
			char pathEXPtmp[2048];
			//Export SOM images if path not NULL 
			if (pathEXP != NULL)
			{
				printf("Saving som: %s(-LR/-HR)\n", pathEXP);
				for (unsigned int y = 0; y < SOM_SIDE; y++)
					for (unsigned int x = 0; x < SOM_SIDE; x++)
					{

						sprintf(pathEXPtmp, "%s_%d-%d", pathEXP,y,x);
						arraySOM[y][x]->exportSOMImage(pathEXPtmp, 127.5, 96);
					}
			}
			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

int main(int argc, char*argv[])
{
	const char *pathSOM = NULL;
	const char *pathLR = NULL;
	const char *pathHR = NULL;
	const char *pathEXP = NULL;
	bool train = false;
	bool load = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strstr(argv[i], "-sv:") == argv[i])
		{
			pathSOM = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-ld") == 0)
		{
			load = true;
		}
		else if (strstr(argv[i], "-e:") == argv[i])
		{
			pathEXP = argv[i] + 3;
		}
		else if (strstr(argv[i], "-lr:") == argv[i])
		{
			pathLR = argv[i] + 4;
		}
		else if (strstr(argv[i], "-hr:") == argv[i])
		{
			pathHR = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-test") == 0)
		{
			train = false;
			load = true;
		}
		else if (strcmp(argv[i], "-learn") == 0)
		{
			train = true;
		}
		else
		{
			printf("Unknown argument: %s\n", argv[i]);
			return -1;
		}
	}
	// CREATE SOM
	printf("-- Superresolution SOM --\n");
	SOM<SOM_SIDE, LR_SIDE, HR_SIDE> som (SOM_STEP, SOM1_SIGMA);
	SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE];
	for (unsigned int y = 0; y < SOM_SIDE; y++)
		for (unsigned int x = 0; x < SOM_SIDE; x++)
			arraySOM[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP,SOM1_SIGMA);
	printf("SOMs created\n");

	// LOAD SOM
	if (load && (pathSOM != NULL) ) 
	{
		loadSOM(som, arraySOM, pathSOM);
		printf("SOMs loaded\n");
	}

	// START //

	if (train)	// TRAINING
	{
		if (pathSOM == NULL)
		{
			printf("There is no path to save the SOM\n");
			return -1;
		}
		printf("Training model...\n");
		trainSOM(som, arraySOM, pathLR, pathHR, pathSOM, pathEXP);
	}
	else		// TESTING
	{
		printf("Testing model...\n");
		testSOM(som, arraySOM, pathLR, pathHR);
	}

	// Free memory
	for (unsigned int y = 0; y < SOM_SIDE; y++)
		for (unsigned int x = 0; x < SOM_SIDE; x++)
			delete arraySOM[y][x];
	printf("End.\n");
}

#endif

//		     //
// 3 LAYERS //
//		   //

#if NUM_LAYERS == 3
#define SOM_SIZE SOM_SIDE*SOM_SIDE

void saveSOM(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM[SOM_SIDE][SOM_SIDE], SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM2[SOM_SIZE][SOM_SIZE], const char* path)
{
	FILE *file = NULL;
	char tmpPath[2048];
	sprintf(tmpPath, "%s.tmp", path);
	fopen_s(&file, tmpPath, "wb");
	if (file)
	{
		som.save(file);
		fflush(file);

		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->save(file);
				fflush(file);
			}

		const double size = SOM_SIDE * SOM_SIDE;
		for (unsigned int y = 0; y < size; ++y)
			for (unsigned int x = 0; x < size; ++x)
			{
				arraySOM2[y][x]->save(file);
				fflush(file);
			}

		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
	remove(path);
	rename(tmpPath, path);

}

void loadSOM(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE], SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM2[SOM_SIZE][SOM_SIZE], const char* path)
{
	FILE *file = NULL;
	fopen_s(&file, path, "rb");
	if (file)
	{
		som.load(file);
		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->load(file);
			}

		const double size = SOM_SIDE * SOM_SIDE;
		for (unsigned int y = 0; y < size; ++y)
			for (unsigned int x = 0; x < size; ++x)
			{
				arraySOM2[y][x]->load(file);
			}

		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}

void testImage(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM[SOM_SIDE][SOM_SIDE], SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM2[SOM_SIZE][SOM_SIZE], const Image &lrImg, const char* path)
{
	// Image to save the SOM version of the LR-image
	Image tempLRImg(lrImg.getWidth(), lrImg.getHeigth());
	Image scoresLo(lrImg.getWidth(), lrImg.getHeigth());
	// Dimensions of the LRImage
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;

	// Full destination HR-image and scores
	const unsigned int heightHR = (maxHeightLR + LR_SIDE) * 2;
	const unsigned int widthHR = (maxWidthLR + LR_SIDE) * 2;
	Image hrImg(widthHR, heightHR);
	Image scoresHi(widthHR, heightHR);
	Patch<LR_SIDE> lrPatch = Patch<LR_SIDE>();
	Patch<HR_SIDE> scores = Patch<HR_SIDE>();
	Patch<HR_SIDE> hrPatch = Patch<HR_SIDE>();

	// Processing
	for (unsigned int y = 0; y < maxHeightLR; ++y)
		for (unsigned int x = 0; x < maxWidthLR; ++x)
		{
			// Load current patch
			lrPatch.readPatch(lrImg, y, x);
			const double avg = lrPatch.getAvg();
			const double dev = lrPatch.getDev();

			// Tests patch
			unsigned int best_i, best_j;
			som.getBest(best_i, best_j, lrPatch); // Gets winner SOM of layer 2 from Base SOM
			unsigned int best_i2, best_j2;
			arraySOM[best_i][best_j]->getBest(best_i2,best_j2,lrPatch);// Gets winner SOM of layer 3 from second layer
			arraySOM2[best_i*SOM_SIDE+best_i2][best_j*SOM_SIDE+best_j2]->test(lrPatch, hrPatch, &scores); // Tests patch 
			hrPatch.addPatch(hrImg, 2 * y, 2 * x);
			scores.addPatch(scoresHi, 2 * y, 2 * x);
		}
	for (unsigned int y = 0; y < heightHR; ++y)
		for (unsigned int x = 0; x < widthHR; ++x)
			hrImg.setValue(hrImg.getImage()[y][x] / scoresHi.getImage()[y][x], y, x);
	hrImg.exportImagePGM(path);
}

void trainImage(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE], SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM2[SOM_SIZE][SOM_SIZE], const Image &lrImg, const Image &hrImg)
{

	// Checks if the high-resoluton image is 2 times the low-resolution image
	if (2 * lrImg.getHeigth() != hrImg.getHeigth() || 2 * lrImg.getWidth() != hrImg.getWidth())
	{
		printf("ERROR: Low resolution image dimensions doesnt match high resolution image");
		return;
	}

	// Get max dimensions for the low-resolution Image
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	printf("LRwidth:%u(%u) LRheight:%u(%u)\n", maxWidthLR + LR_SIDE, lrImg.getWidth(), maxHeightLR + LR_SIDE, lrImg.getHeigth());

	// New cell to load the Patches to train
	Cell<LR_SIDE, HR_SIDE> cell = Cell<LR_SIDE, HR_SIDE>(1);

	for (unsigned int y = 0; y < maxHeightLR; y += SOM_STEP)
	{
		for (unsigned int x = 0; x < maxWidthLR; x += SOM_STEP)
		{
			// Load current LR-patch
			cell.getLowPatch().readPatch(lrImg, y, x);

			// Load current HR-patch
			cell.getHighPatch().readPatch(hrImg, 2 * y, 2 * x);

			// Train current patch
			unsigned int best_i, best_j;
			som.train(cell, &best_i, &best_j);
			unsigned int best_i2, best_j2;
			arraySOM[best_i][best_j]->train(cell, &best_i2, &best_j2);
			arraySOM2[best_i*SOM_SIDE+best_i2][best_j*SOM_SIDE+best_j2]->train(cell);
		}
	}
}

void testSOM(const SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM[SOM_SIDE][SOM_SIDE], SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *const arraySOM2[SOM_SIZE][SOM_SIZE], const char* pathLR, const char* outputPath)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);

	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			Image lrImg(tmpLR);
			sprintf(tmpHR, "%s/%s", outputPath, pgm_file.name);
			printf("\nTesting image %d:\n  * %s\n  *output:  %s\n", count, tmpLR, tmpHR);

			// Tests full image
			const clock_t  clkIni = clock();
			testImage(som, arraySOM,arraySOM2, lrImg, tmpHR);
			printf("  * Tested image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

void trainSOM(SOM<SOM_SIDE, LR_SIDE, HR_SIDE> &som, SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE], SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM2[SOM_SIZE][SOM_SIZE], const char* pathLR, const char* pathHR, const char* pathSOM, const char* pathEXP)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);
	printf("Searching .pgm in %s\n", pathLR);
	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			sprintf(tmpHR, "%s/%s", pathHR, pgm_file.name);
			printf("\nTraining image %d:\n  * %s\n  * %s\n", count, tmpLR, tmpHR);

			// Loads both low-resolution and high-resolution images
			Image lrImg(tmpLR);
			Image hrImg(tmpHR);

			// Train images
			const clock_t  clkIni = clock();
			trainImage(som, arraySOM,arraySOM2, lrImg, hrImg);
			printf("  * Trained image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			//Save SOM after each image
			saveSOM(som, arraySOM,arraySOM2, pathSOM);
			char pathEXPtmp[2048];
			//Export SOM images if path not NULL 
			if (pathEXP != NULL)
			{
				printf("Saving som: %s(-LR/-HR)\n", pathEXP);
				for (unsigned int y = 0; y < SOM_SIDE; y++)
					for (unsigned int x = 0; x < SOM_SIDE; x++)
					{

						sprintf(pathEXPtmp, "%s_%d-%d", pathEXP, y, x);
						arraySOM[y][x]->exportSOMImage(pathEXPtmp, 127.5, 96);
					}
			}
			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

int main(int argc, char*argv[])
{
	const char *pathSOM = NULL;
	const char *pathLR = NULL;
	const char *pathHR = NULL;
	const char *pathEXP = NULL;
	bool train = false;
	bool load = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strstr(argv[i], "-sv:") == argv[i])
		{
			pathSOM = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-ld") == 0)
		{
			load = true;
		}
		else if (strstr(argv[i], "-e:") == argv[i])
		{
			pathEXP = argv[i] + 3;
		}
		else if (strstr(argv[i], "-lr:") == argv[i])
		{
			pathLR = argv[i] + 4;
		}
		else if (strstr(argv[i], "-hr:") == argv[i])
		{
			pathHR = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-test") == 0)
		{
			train = false;
			load = true;
		}
		else if (strcmp(argv[i], "-learn") == 0)
		{
			train = true;
		}
		else
		{
			printf("Unknown argument: %s\n", argv[i]);
			return -1;
		}
	}
	// CREATE SOM
	printf("-- Superresolution SOM --\n");
	SOM<SOM_SIDE, LR_SIDE, HR_SIDE> som(SOM_STEP, SOM1_SIGMA); // Base SOM
	SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE]; // Second Layer SOMs
	SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM2[SOM_SIZE][SOM_SIZE]; // Third Layer SOMs

	for (unsigned int y = 0; y < SOM_SIDE; y++)
		for (unsigned int x = 0; x < SOM_SIDE; x++)
			arraySOM[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);

	for (unsigned int y = 0; y < SOM_SIZE; y++)
		for (unsigned int x = 0; x < SOM_SIZE; x++)
			arraySOM2[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	

	printf("SOMs created\n");

	// LOAD SOM
	if (load && (pathSOM != NULL))
	{
		loadSOM(som, arraySOM, arraySOM2, pathSOM);
		printf("SOMs loaded\n");
	}

	// START //

	if (train)	// TRAINING
	{
		if (pathSOM == NULL)
		{
			printf("There is no path to save the SOM\n");
			return -1;
		}
		printf("Training model...\n");
		trainSOM(som, arraySOM, arraySOM2, pathLR, pathHR, pathSOM, pathEXP);
	}
	else		// TESTING
	{
		printf("Testing model...\n");
		testSOM(som, arraySOM, arraySOM2, pathLR, pathHR);
	}

	// Free memory
	for (unsigned int y = 0; y < SOM_SIDE; y++)
		for (unsigned int x = 0; x < SOM_SIDE; x++)
			delete arraySOM[y][x];

	double size = SOM_SIDE * SOM_SIDE;
	for (unsigned int y = 0; y < size; y++)
		for (unsigned int x = 0; x < size; x++)
			delete arraySOM2[y][x];

		
	printf("End.\n");
}

#endif


//		     //
// 5 LAYERS //
//		   //

#if NUM_LAYERS == 5
#define SOM_SIZE SOM_SIDE * SOM_SIDE
#define SOM_SIDE4 SOM_SIZE * SOM_SIDE
#define SOM_SIDE5 SOM_SIDE4 * SOM_SIDE

SOM<SOM_SIDE, LR_SIDE, HR_SIDE> som(SOM_STEP, SOM1_SIGMA); // Base SOM
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE]; // Second Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM2[SOM_SIZE][SOM_SIZE]; // Third Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM3[SOM_SIDE4][SOM_SIDE4]; // Fourth Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM4[SOM_SIDE5][SOM_SIDE5]; // Fifth Layer SOMs

void saveSOM(const char* path)
{
	FILE *file = NULL;
	char tmpPath[2048];
	sprintf(tmpPath, "%s.tmp", path);
	fopen_s(&file, tmpPath, "wb");
	if (file)
	{
		som.save(file);
		fflush(file);

		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->save(file);
				fflush(file);
			}

		const double size = SOM_SIDE * SOM_SIDE;
		for (unsigned int y = 0; y < SOM_SIZE; ++y)
			for (unsigned int x = 0; x < SOM_SIZE; ++x)
			{
				arraySOM2[y][x]->save(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE4; ++y)
			for (unsigned int x = 0; x < SOM_SIDE4; ++x)
			{
				arraySOM3[y][x]->save(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE5; ++y)
			for (unsigned int x = 0; x < SOM_SIDE5; ++x)
			{
				arraySOM4[y][x]->save(file);
				fflush(file);
			}

		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
	remove(path);
	rename(tmpPath, path);

}

void loadSOM(const char* path)
{
	FILE *file = NULL;
	fopen_s(&file, path, "rb");
	if (file)
	{
		som.load(file);
		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->load(file);
			}

		const double size = SOM_SIDE * SOM_SIDE;
		for (unsigned int y = 0; y < size; ++y)
			for (unsigned int x = 0; x < size; ++x)
			{
				arraySOM2[y][x]->load(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE4; ++y)
			for (unsigned int x = 0; x < SOM_SIDE4; ++x)
			{
				arraySOM3[y][x]->load(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE5; ++y)
			for (unsigned int x = 0; x < SOM_SIDE5; ++x)
			{
				arraySOM4[y][x]->load(file);
				fflush(file);
			}

		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}


void testImage(const Image &lrImg, const char* path)
{
	// Image to save the SOM version of the LR-image
	Image tempLRImg(lrImg.getWidth(), lrImg.getHeigth());
	Image scoresLo(lrImg.getWidth(), lrImg.getHeigth());
	// Dimensions of the LRImage
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;

	// Full destination HR-image and scores
	const unsigned int heightHR = (maxHeightLR + LR_SIDE) * 2;
	const unsigned int widthHR = (maxWidthLR + LR_SIDE) * 2;
	Image hrImg(widthHR, heightHR);
	Image scoresHi(widthHR, heightHR);
	Patch<LR_SIDE> lrPatch = Patch<LR_SIDE>();
	Patch<HR_SIDE> scores = Patch<HR_SIDE>();
	Patch<HR_SIDE> hrPatch = Patch<HR_SIDE>();

	// Processing
	for (unsigned int y = 0; y < maxHeightLR; ++y)
		for (unsigned int x = 0; x < maxWidthLR; ++x)
		{
			// Load current patch
			lrPatch.readPatch(lrImg, y, x);
			const double avg = lrPatch.getAvg();
			const double dev = lrPatch.getDev();

			// Tests patch
			unsigned int best_i, best_j;
			som.getBest(best_i, best_j, lrPatch); // Gets winner SOM of layer 2 from Base SOM

			unsigned int best_i2, best_j2;
			arraySOM[best_i][best_j]->getBest(best_i2, best_j2, lrPatch);// Gets winner SOM of layer 3 from second layer

			unsigned int best_i3, best_j3;
			unsigned int i2 = best_i * SOM_SIDE + best_i2;
			unsigned int j2 = best_j * SOM_SIDE + best_j2;
			arraySOM2[i2][j2]->getBest(best_i3, best_j3, lrPatch);

			unsigned int best_i4, best_j4;
			unsigned int i3 = i2 * SOM_SIDE + best_i3;
			unsigned int j3 = j2 * SOM_SIDE + best_j3;
			arraySOM3[i3][j3]->getBest(best_i4, best_j4, lrPatch);


			arraySOM4[i3*SOM_SIDE + best_i4][j3*SOM_SIDE + best_j4]->test(lrPatch, hrPatch, &scores); // Tests patch 

			hrPatch.addPatch(hrImg, 2 * y, 2 * x);
			scores.addPatch(scoresHi, 2 * y, 2 * x);
		}
	for (unsigned int y = 0; y < heightHR; ++y)
		for (unsigned int x = 0; x < widthHR; ++x)
			hrImg.setValue(hrImg.getImage()[y][x] / scoresHi.getImage()[y][x], y, x);
	hrImg.exportImagePGM(path);
}

void trainImage(const Image &lrImg, const Image &hrImg)
{

	// Checks if the high-resoluton image is 2 times the low-resolution image
	if (2 * lrImg.getHeigth() != hrImg.getHeigth() || 2 * lrImg.getWidth() != hrImg.getWidth())
	{
		printf("ERROR: Low resolution image dimensions doesnt match high resolution image");
		return;
	}

	// Get max dimensions for the low-resolution Image
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	printf("LRwidth:%u(%u) LRheight:%u(%u)\n", maxWidthLR + LR_SIDE, lrImg.getWidth(), maxHeightLR + LR_SIDE, lrImg.getHeigth());

	// New cell to load the Patches to train
	Cell<LR_SIDE, HR_SIDE> cell = Cell<LR_SIDE, HR_SIDE>(1);

	for (unsigned int y = 0; y < maxHeightLR; y += SOM_STEP)
	{
		for (unsigned int x = 0; x < maxWidthLR; x += SOM_STEP)
		{
			// Load current LR-patch
			cell.getLowPatch().readPatch(lrImg, y, x);

			// Load current HR-patch
			cell.getHighPatch().readPatch(hrImg, 2 * y, 2 * x);

			// Train current patch
			unsigned int best_i, best_j;
			som.train(cell, &best_i, &best_j);

			unsigned int best_i2, best_j2;
			arraySOM[best_i][best_j]->train(cell, &best_i2, &best_j2);

			unsigned int best_i3, best_j3;
			unsigned int i2 = best_i * SOM_SIDE + best_i2;
			unsigned int j2 = best_j * SOM_SIDE + best_j2;
			arraySOM2[i2][j2]->train(cell, &best_i3, &best_j3);

			unsigned int best_i4, best_j4;
			unsigned int i3 = i2 * SOM_SIDE + best_i3;
			unsigned int j3 = j2 * SOM_SIDE + best_j3;
			arraySOM3[i3][j3]->train(cell, &best_i4, &best_j4);


			arraySOM4[i3*SOM_SIDE + best_i4][j3*SOM_SIDE + best_j4]->train(cell);
		}
	}
}

void testSOM(const char* pathLR, const char* outputPath)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);

	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			Image lrImg(tmpLR);
			sprintf(tmpHR, "%s/%s", outputPath, pgm_file.name);
			printf("\nTesting image %d:\n  * %s\n  *output:  %s\n", count, tmpLR, tmpHR);

			// Tests full image
			const clock_t  clkIni = clock();
			testImage(lrImg, tmpHR);
			printf("  * Tested image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

void trainSOM(const char* pathLR, const char* pathHR, const char* pathSOM, const char* pathEXP)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);
	printf("Searching .pgm in %s\n", pathLR);
	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			sprintf(tmpHR, "%s/%s", pathHR, pgm_file.name);
			printf("\nTraining image %d:\n  * %s\n  * %s\n", count, tmpLR, tmpHR);

			// Loads both low-resolution and high-resolution images
			Image lrImg(tmpLR);
			Image hrImg(tmpHR);

			// Train images
			const clock_t  clkIni = clock();
			trainImage(lrImg, hrImg);
			printf("  * Trained image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			//Save SOM after each image
			saveSOM(pathSOM);
			char pathEXPtmp[2048];
			//Export SOM images if path not NULL 
			if (pathEXP != NULL)
			{
				printf("Saving som: %s(-LR/-HR)\n", pathEXP);
				for (unsigned int y = 0; y < SOM_SIDE; y++)
					for (unsigned int x = 0; x < SOM_SIDE; x++)
					{

						sprintf(pathEXPtmp, "%s_%d-%d", pathEXP, y, x);
						arraySOM[y][x]->exportSOMImage(pathEXPtmp, 127.5, 96);
					}
			}
			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

int main(int argc, char*argv[])
{
	const char *pathSOM = NULL;
	const char *pathLR = NULL;
	const char *pathHR = NULL;
	const char *pathEXP = NULL;
	bool train = false;
	bool load = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strstr(argv[i], "-sv:") == argv[i])
		{
			pathSOM = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-ld") == 0)
		{
			load = true;
		}
		else if (strstr(argv[i], "-e:") == argv[i])
		{
			pathEXP = argv[i] + 3;
		}
		else if (strstr(argv[i], "-lr:") == argv[i])
		{
			pathLR = argv[i] + 4;
		}
		else if (strstr(argv[i], "-hr:") == argv[i])
		{
			pathHR = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-test") == 0)
		{
			train = false;
			load = true;
		}
		else if (strcmp(argv[i], "-learn") == 0)
		{
			train = true;
		}
		else
		{
			printf("Unknown argument: %s\n", argv[i]);
			return -1;
		}
	}
	// CREATE SOM
	printf("-- Superresolution SOM --\n");


	for (unsigned int y = 0; y < SOM_SIDE; ++y)
		for (unsigned int x = 0; x < SOM_SIDE; ++x)
			arraySOM[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM1 --\n");

	for (unsigned int y = 0; y < SOM_SIZE; ++y)
		for (unsigned int x = 0; x < SOM_SIZE; ++x)
			arraySOM2[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM2 --\n");

	for (unsigned int y = 0; y < SOM_SIDE4; ++y)
		for (unsigned int x = 0; x < SOM_SIDE4; ++x)
			arraySOM3[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM3 --\n");

	for (unsigned int y = 0; y < SOM_SIDE5; ++y)
		for (unsigned int x = 0; x < SOM_SIDE5; ++x)
			arraySOM4[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM4 --\n");

	printf("SOMs created\n");

	// LOAD SOM
	if (load && (pathSOM != NULL))
	{
		loadSOM(pathSOM);
		printf("SOMs loaded\n");
	}

	// START //

	if (train)	// TRAINING
	{
		if (pathSOM == NULL)
		{
			printf("There is no path to save the SOM\n");
			return -1;
		}
		printf("Training model...\n");
		trainSOM(pathLR, pathHR, pathSOM, pathEXP);
	}
	else		// TESTING
	{
		printf("Testing model...\n");
		testSOM(pathLR, pathHR);
	}

	// Free memory
	for (unsigned int y = 0; y < SOM_SIDE; y++)
		for (unsigned int x = 0; x < SOM_SIDE; x++)
			delete arraySOM[y][x];

	for (unsigned int y = 0; y < SOM_SIZE; y++)
		for (unsigned int x = 0; x < SOM_SIZE; x++)
			delete arraySOM2[y][x];

	for (unsigned int y = 0; y < SOM_SIDE4; y++)
		for (unsigned int x = 0; x < SOM_SIDE4; x++)
			delete arraySOM3[y][x];


	for (unsigned int y = 0; y < SOM_SIDE5; y++)
		for (unsigned int x = 0; x < SOM_SIDE5; x++)
			delete arraySOM4[y][x];


	printf("End.\n");
}

#endif

//		     //
// 7 LAYERS //
//		   //

#if NUM_LAYERS == 7
#define SOM_SIZE SOM_SIDE * SOM_SIDE
#define SOM_SIDE4 SOM_SIZE * SOM_SIDE
#define SOM_SIDE5 SOM_SIDE4 * SOM_SIDE
#define SOM_SIDE6 SOM_SIDE5 * SOM_SIDE
#define SOM_SIDE7 SOM_SIDE6 * SOM_SIDE


SOM<SOM_SIDE, LR_SIDE, HR_SIDE> som(SOM_STEP, SOM1_SIGMA); // Base SOM
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM[SOM_SIDE][SOM_SIDE]; // Second Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM2[SOM_SIZE][SOM_SIZE]; // Third Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM3[SOM_SIDE4][SOM_SIDE4]; // Fourth Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM4[SOM_SIDE5][SOM_SIDE5]; // Fifth Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM5[SOM_SIDE6][SOM_SIDE6]; // 6 Layer SOMs
SOM<SOM_SIDE, LR_SIDE, HR_SIDE> *arraySOM6[SOM_SIDE7][SOM_SIDE7]; // 7 Layer SOMs


void saveSOM(const char* path)
{
	FILE *file = NULL;
	char tmpPath[2048];
	sprintf(tmpPath, "%s.tmp", path);
	fopen_s(&file, tmpPath, "wb");
	if (file)
	{
		som.save(file);
		fflush(file);

		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->save(file);
				fflush(file);
			}

		const double size = SOM_SIDE * SOM_SIDE;
		for (unsigned int y = 0; y < SOM_SIZE; ++y)
			for (unsigned int x = 0; x < SOM_SIZE; ++x)
			{
				arraySOM2[y][x]->save(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE4; ++y)
			for (unsigned int x = 0; x < SOM_SIDE4; ++x)
			{
				arraySOM3[y][x]->save(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE5; ++y)
			for (unsigned int x = 0; x < SOM_SIDE5; ++x)
			{
				arraySOM4[y][x]->save(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE6; ++y)
			for (unsigned int x = 0; x < SOM_SIDE6; ++x)
			{
				arraySOM5[y][x]->save(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE7; ++y)
			for (unsigned int x = 0; x < SOM_SIDE7; ++x)
			{
				arraySOM6[y][x]->save(file);
				fflush(file);
			}

		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
	remove(path);
	rename(tmpPath, path);

}

void loadSOM(const char* path)
{
	FILE *file = NULL;
	fopen_s(&file, path, "rb");
	if (file)
	{
		som.load(file);
		for (unsigned int y = 0; y < SOM_SIDE; ++y)
			for (unsigned int x = 0; x < SOM_SIDE; ++x)
			{
				arraySOM[y][x]->load(file);
			}

		const double size = SOM_SIDE * SOM_SIDE;
		for (unsigned int y = 0; y < size; ++y)
			for (unsigned int x = 0; x < size; ++x)
			{
				arraySOM2[y][x]->load(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE4; ++y)
			for (unsigned int x = 0; x < SOM_SIDE4; ++x)
			{
				arraySOM3[y][x]->load(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE5; ++y)
			for (unsigned int x = 0; x < SOM_SIDE5; ++x)
			{
				arraySOM4[y][x]->load(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE6; ++y)
			for (unsigned int x = 0; x < SOM_SIDE6; ++x)
			{
				arraySOM5[y][x]->load(file);
				fflush(file);
			}

		for (unsigned int y = 0; y < SOM_SIDE7; ++y)
			for (unsigned int x = 0; x < SOM_SIDE7; ++x)
			{
				arraySOM6[y][x]->load(file);
				fflush(file);
			}

		fclose(file);
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}


void testImage(const Image &lrImg, const char* path)
{
	// Image to save the SOM version of the LR-image
	Image tempLRImg(lrImg.getWidth(), lrImg.getHeigth());
	Image scoresLo(lrImg.getWidth(), lrImg.getHeigth());
	// Dimensions of the LRImage
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;

	// Full destination HR-image and scores
	const unsigned int heightHR = (maxHeightLR + LR_SIDE) * 2;
	const unsigned int widthHR = (maxWidthLR + LR_SIDE) * 2;
	Image hrImg(widthHR, heightHR);
	Image scoresHi(widthHR, heightHR);
	Patch<LR_SIDE> lrPatch = Patch<LR_SIDE>();
	Patch<HR_SIDE> scores = Patch<HR_SIDE>();
	Patch<HR_SIDE> hrPatch = Patch<HR_SIDE>();

	// Processing
	for (unsigned int y = 0; y < maxHeightLR; ++y)
		for (unsigned int x = 0; x < maxWidthLR; ++x)
		{
			// Load current patch
			lrPatch.readPatch(lrImg, y, x);
			const double avg = lrPatch.getAvg();
			const double dev = lrPatch.getDev();

			// Tests patch
			unsigned int best_i, best_j;
			som.getBest(best_i, best_j, lrPatch); // Gets winner SOM of layer 2 from Base SOM

			unsigned int best_i2, best_j2;
			arraySOM[best_i][best_j]->getBest(best_i2, best_j2, lrPatch);// Gets winner SOM of layer 3 from second layer

			unsigned int best_i3, best_j3;
			unsigned int i2 = best_i * SOM_SIDE + best_i2;
			unsigned int j2 = best_j * SOM_SIDE + best_j2;
			arraySOM2[i2][j2]->getBest(best_i3, best_j3, lrPatch);

			unsigned int best_i4, best_j4;
			unsigned int i3 = i2 * SOM_SIDE + best_i3;
			unsigned int j3 = j2 * SOM_SIDE + best_j3;
			arraySOM3[i3][j3]->getBest(best_i4, best_j4, lrPatch);

			unsigned int best_i5, best_j5;
			unsigned int i4 = i3 * SOM_SIDE + best_i4;
			unsigned int j4 = j3 * SOM_SIDE + best_j4;
			arraySOM4[i4][j4]->getBest(best_i5, best_j5, lrPatch);

			unsigned int best_i6, best_j6;
			unsigned int i5 = i4 * SOM_SIDE + best_i5;
			unsigned int j5 = j4 * SOM_SIDE + best_j5;
			arraySOM5[i5][j5]->getBest(best_i6, best_j6, lrPatch);

//			unsigned int best_i7, best_j7;
			unsigned int i6 = i5 * SOM_SIDE + best_i6;
			unsigned int j6 = j5 * SOM_SIDE + best_j6;
//			arraySOM6[i6][j6]->getBest(best_i7, best_j7, lrPatch);
			arraySOM6[i6][j6]->test(lrPatch, hrPatch, &scores);



			hrPatch.addPatch(hrImg, 2 * y, 2 * x);
			scores.addPatch(scoresHi, 2 * y, 2 * x);
		}
	for (unsigned int y = 0; y < heightHR; ++y)
		for (unsigned int x = 0; x < widthHR; ++x)
			hrImg.setValue(hrImg.getImage()[y][x] / scoresHi.getImage()[y][x], y, x);
	hrImg.exportImagePGM(path);
}

void trainImage(const Image &lrImg, const Image &hrImg)
{

	// Checks if the high-resoluton image is 2 times the low-resolution image
	if (2 * lrImg.getHeigth() != hrImg.getHeigth() || 2 * lrImg.getWidth() != hrImg.getWidth())
	{
		printf("ERROR: Low resolution image dimensions doesnt match high resolution image");
		return;
	}

	// Get max dimensions for the low-resolution Image
	const unsigned int maxHeightLR = ((lrImg.getHeigth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	const unsigned int maxWidthLR = ((lrImg.getWidth() - LR_SIDE) / LR_SIDE) * LR_SIDE;
	printf("LRwidth:%u(%u) LRheight:%u(%u)\n", maxWidthLR + LR_SIDE, lrImg.getWidth(), maxHeightLR + LR_SIDE, lrImg.getHeigth());

	// New cell to load the Patches to train
	Cell<LR_SIDE, HR_SIDE> cell = Cell<LR_SIDE, HR_SIDE>(1);

	for (unsigned int y = 0; y < maxHeightLR; y += SOM_STEP)
	{
		for (unsigned int x = 0; x < maxWidthLR; x += SOM_STEP)
		{
			// Load current LR-patch
			cell.getLowPatch().readPatch(lrImg, y, x);

			// Load current HR-patch
			cell.getHighPatch().readPatch(hrImg, 2 * y, 2 * x);

			// Train current patch
			unsigned int best_i, best_j;
			som.train(cell, &best_i, &best_j);

			unsigned int best_i2, best_j2;
			arraySOM[best_i][best_j]->train(cell, &best_i2, &best_j2);

			unsigned int best_i3, best_j3;
			unsigned int i2 = best_i * SOM_SIDE + best_i2;
			unsigned int j2 = best_j * SOM_SIDE + best_j2;
			arraySOM2[i2][j2]->train(cell, &best_i3, &best_j3);

			unsigned int best_i4, best_j4;
			unsigned int i3 = i2 * SOM_SIDE + best_i3;
			unsigned int j3 = j2 * SOM_SIDE + best_j3;
			arraySOM3[i3][j3]->train(cell, &best_i4, &best_j4);

			unsigned int best_i5, best_j5;
			unsigned int i4 = i3 * SOM_SIDE + best_i4;
			unsigned int j4 = j3 * SOM_SIDE + best_j4;
			arraySOM4[i4][j4]->train(cell, &best_i5, &best_j5);

			unsigned int best_i6, best_j6;
			unsigned int i5 = i4 * SOM_SIDE + best_i5;
			unsigned int j5 = j4 * SOM_SIDE + best_j5;
			arraySOM5[i5][j5]->train(cell, &best_i6, &best_j6);

			//unsigned int best_i7, best_j7;
			unsigned int i6 = i5 * SOM_SIDE + best_i6;
			unsigned int j6 = j5 * SOM_SIDE + best_j6;
			//arraySOM6[i6][j6]->train(cell, &best_i7, &best_j7);
			arraySOM6[i6][j6]->train(cell);



		}
	}
}

void testSOM(const char* pathLR, const char* outputPath)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);

	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			Image lrImg(tmpLR);
			sprintf(tmpHR, "%s/%s", outputPath, pgm_file.name);
			printf("\nTesting image %d:\n  * %s\n  *output:  %s\n", count, tmpLR, tmpHR);

			// Tests full image
			const clock_t  clkIni = clock();
			testImage(lrImg, tmpHR);
			printf("  * Tested image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

void trainSOM(const char* pathLR, const char* pathHR, const char* pathSOM, const char* pathEXP)
{
	char tmpLR[2048];
	char tmpHR[2048];
	sprintf(tmpLR, "%s/*.pgm", pathLR);
	printf("Searching .pgm in %s\n", pathLR);
	struct _finddata_t pgm_file;
	intptr_t hFile;

	// Find first .pgm in the path
	unsigned int count = 1;
	if ((hFile = _findfirst(tmpLR, &pgm_file)) == -1L)
		printf("No *.pgm files in current directory!\n");
	else
	{
		do
		{
			sprintf(tmpLR, "%s/%s", pathLR, pgm_file.name);
			sprintf(tmpHR, "%s/%s", pathHR, pgm_file.name);
			printf("\nTraining image %d:\n  * %s\n  * %s\n", count, tmpLR, tmpHR);

			// Loads both low-resolution and high-resolution images
			Image lrImg(tmpLR);
			Image hrImg(tmpHR);

			// Train images
			const clock_t  clkIni = clock();
			trainImage(lrImg, hrImg);
			printf("  * Trained image %s in %f secs.", pgm_file.name, CLK_TO_SEC(clock() - clkIni));

			//Save SOM after each image
			saveSOM(pathSOM);
			char pathEXPtmp[2048];
			//Export SOM images if path not NULL 
			if (pathEXP != NULL)
			{
				printf("Saving som: %s(-LR/-HR)\n", pathEXP);
				for (unsigned int y = 0; y < SOM_SIDE; y++)
					for (unsigned int x = 0; x < SOM_SIDE; x++)
					{

						sprintf(pathEXPtmp, "%s_%d-%d", pathEXP, y, x);
						arraySOM[y][x]->exportSOMImage(pathEXPtmp, 127.5, 96);
					}
			}
			++count;
		} while (_findnext(hFile, &pgm_file) == 0);
		_findclose(hFile);
	}
}

int main(int argc, char*argv[])
{
	const char *pathSOM = NULL;
	const char *pathLR = NULL;
	const char *pathHR = NULL;
	const char *pathEXP = NULL;
	bool train = false;
	bool load = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strstr(argv[i], "-sv:") == argv[i])
		{
			pathSOM = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-ld") == 0)
		{
			load = true;
		}
		else if (strstr(argv[i], "-e:") == argv[i])
		{
			pathEXP = argv[i] + 3;
		}
		else if (strstr(argv[i], "-lr:") == argv[i])
		{
			pathLR = argv[i] + 4;
		}
		else if (strstr(argv[i], "-hr:") == argv[i])
		{
			pathHR = argv[i] + 4;
		}
		else if (strcmp(argv[i], "-test") == 0)
		{
			train = false;
			load = true;
		}
		else if (strcmp(argv[i], "-learn") == 0)
		{
			train = true;
		}
		else
		{
			printf("Unknown argument: %s\n", argv[i]);
			return -1;
		}
	}
	// CREATE SOM
	printf("-- Superresolution SOM --\n");


	for (unsigned int y = 0; y < SOM_SIDE; ++y)
		for (unsigned int x = 0; x < SOM_SIDE; ++x)
			arraySOM[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM2 --\n");

	for (unsigned int y = 0; y < SOM_SIZE; ++y)
		for (unsigned int x = 0; x < SOM_SIZE; ++x)
			arraySOM2[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM3 --\n");

	for (unsigned int y = 0; y < SOM_SIDE4; ++y)
		for (unsigned int x = 0; x < SOM_SIDE4; ++x)
			arraySOM3[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM4 --\n");

	for (unsigned int y = 0; y < SOM_SIDE5; ++y)
		for (unsigned int x = 0; x < SOM_SIDE5; ++x)
			arraySOM4[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM5 --\n");

	for (unsigned int y = 0; y < SOM_SIDE6; ++y)
		for (unsigned int x = 0; x < SOM_SIDE6; ++x)
			arraySOM5[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM6 --\n");

	for (unsigned int y = 0; y < SOM_SIDE7; ++y)
		for (unsigned int x = 0; x < SOM_SIDE7; ++x)
			arraySOM6[y][x] = new SOM<SOM_SIDE, LR_SIDE, HR_SIDE>(SOM_STEP, SOM1_SIGMA);
	printf("-- SOM7 --\n");


	printf("SOMs created\n");

	// LOAD SOM
	if (load && (pathSOM != NULL))
	{
		loadSOM(pathSOM);
		printf("SOMs loaded\n");
	}

	// START //

	if (train)	// TRAINING
	{
		if (pathSOM == NULL)
		{
			printf("There is no path to save the SOM\n");
			return -1;
		}
		printf("Training model...\n");
		trainSOM(pathLR, pathHR, pathSOM, pathEXP);
	}
	else		// TESTING
	{
		printf("Testing model...\n");
		testSOM(pathLR, pathHR);
	}

	// Free memory
	for (unsigned int y = 0; y < SOM_SIDE; y++)
		for (unsigned int x = 0; x < SOM_SIDE; x++)
			delete arraySOM[y][x];

	for (unsigned int y = 0; y < SOM_SIZE; y++)
		for (unsigned int x = 0; x < SOM_SIZE; x++)
			delete arraySOM2[y][x];

	for (unsigned int y = 0; y < SOM_SIDE4; y++)
		for (unsigned int x = 0; x < SOM_SIDE4; x++)
			delete arraySOM3[y][x];


	for (unsigned int y = 0; y < SOM_SIDE5; y++)
		for (unsigned int x = 0; x < SOM_SIDE5; x++)
			delete arraySOM4[y][x];

	for (unsigned int y = 0; y < SOM_SIDE6; y++)
		for (unsigned int x = 0; x < SOM_SIDE6; x++)
			delete arraySOM5[y][x];

	for (unsigned int y = 0; y < SOM_SIDE7; y++)
		for (unsigned int x = 0; x < SOM_SIDE7; x++)
			delete arraySOM6[y][x];


	printf("End.\n");
}

#endif