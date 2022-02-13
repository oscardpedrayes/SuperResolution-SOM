#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <thread>
#include <mutex>

#include "Image.h"

#pragma warning (disable : 4996)
# define MAX_THREAD 12

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
SOM<NSIDE,LSIDE,HSIDE>::SOM(const char *path, const unsigned int stp)
	: iter_(0)
	, step_(stp)
{
	srand((unsigned int)time(NULL));
	load(path);
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
SOM<NSIDE,LSIDE,HSIDE>::SOM(const unsigned int stp, double sigma)
	: iter_(0)
	, step_(stp)
{
	srand((unsigned int)time(NULL));
	//Allocates space for matrix
	initializeMatrix(sigma);
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
SOM<NSIDE,LSIDE,HSIDE>::~SOM()
{
	for (unsigned int i = 0; i < NSIDE; ++i)
		for (unsigned int j = 0; j < NSIDE; ++j)
		{
			delete matrix_[i][j];
		}
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
const unsigned long long SOM<NSIDE, LSIDE, HSIDE>::getIter() const
{
	return iter_;
}


template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
void SOM<NSIDE,LSIDE,HSIDE>::initializeMatrix(double sigma)
{
	for (unsigned int i = 0; i < NSIDE; ++i)
		for (unsigned int j = 0; j < NSIDE; ++j)
		{
			matrix_[i][j] = new Cell<LSIDE, HSIDE>(sigma);
		}
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
void SOM<NSIDE,LSIDE,HSIDE>::save(FILE *file) const
{
	if (file)
	{
		unsigned int nside = NSIDE;
		unsigned int lside = LSIDE;
		unsigned int hside = HSIDE;

		fwrite(&nside, sizeof(unsigned int), 1, file);
		fwrite(&lside, sizeof(unsigned int), 1, file);
		fwrite(&hside, sizeof(unsigned int), 1, file);
		fwrite(&iter_, sizeof(unsigned long long), 1, file);
		
		printf("Saved SOM: %u %u %u %llu\n", NSIDE, LSIDE, HSIDE, iter_);

		for (unsigned int i = 0; i < NSIDE; ++i)
			for (unsigned int j = 0; j < NSIDE; ++j)
			{
				for (unsigned int y = 0; y < LSIDE; ++y)
					fwrite(matrix_[i][j]->getLowPatch().getImage()[y], sizeof(double), LSIDE, file);

				for (unsigned int y = 0; y < HSIDE; ++y)
					fwrite(matrix_[i][j]->getHighPatch().getImage()[y], sizeof(double), HSIDE, file);
			}
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
void SOM<NSIDE,LSIDE,HSIDE>::load(FILE *file)
{
	
	if (file)
	{
		unsigned int nside, lside, hside;

		fread(&nside, sizeof(unsigned int), 1, file);
		fread(&lside, sizeof(unsigned int), 1, file);
		fread(&hside, sizeof(unsigned int), 1, file);
		fread(&iter_, sizeof(unsigned long long), 1, file);

		printf("Loaded SOM: %u %u %u %llu\n", nside, lside, hside, iter_);

		initializeMatrix(1);

		for (unsigned int i = 0; i < NSIDE; ++i)
			for (unsigned int j = 0; j < NSIDE; ++j)
			{
				for (unsigned int y = 0; y < LSIDE; ++y)
					fread(matrix_[i][j]->getLowPatch().getImage()[y], sizeof(double), LSIDE, file);

				for (unsigned int y = 0; y < HSIDE; ++y)
					fread(matrix_[i][j]->getHighPatch().getImage()[y], sizeof(double), HSIDE, file);
			}
	}
	else
	{
		printf("ERROR: cannot open file\n");
	}
}

/*
struct CellInfo
{
	std::mutex mtx;
	unsigned int i;
	double bestScore;
	unsigned int best_i, best_j;
	double **img;
	SOM *som;
};

static void threadTest(CellInfo &cellInfo)
{
	for (;;)
	{
		unsigned int curr_i = 0;
		cellInfo.mtx.lock();
		curr_i = cellInfo.i;
		++cellInfo.i;
		cellInfo.mtx.unlock();

		if (curr_i >= cellInfo.som->nside) return;

		double bestScore = 0;
		unsigned int best_j = 0;
		for (unsigned int j = 0; j < cellInfo.som->nside; ++j)
		{
			const double score = cellInfo.som->Compare(cellInfo.som->matrix[curr_i*cellInfo.som->nside + j].lowres, cellInfo.img);
			if (score > bestScore)
			{
				bestScore = score;
				best_j = j;
			}
		}

		cellInfo.mtx.lock();
		if (bestScore > cellInfo.bestScore)
		{
			cellInfo.bestScore = bestScore;
			cellInfo.best_i = curr_i;
			cellInfo.best_j = best_j;
		}
		cellInfo.mtx.unlock();
	}
}
*/

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
double const SOM<NSIDE,LSIDE,HSIDE>::getBest(unsigned int &best_i, unsigned int &best_j,const Patch<LSIDE> &p) const
{
#if 0 //Paralelo  (Mas lento)
	CellInfo cellInfo;
	cellInfo.i = 0;
	cellInfo.bestScore = 0.0;
	cellInfo.best_i = 0;
	cellInfo.best_j = 0;
	cellInfo.img = img;
	cellInfo.som = this;

	std::thread threadPool[MAX_THREAD];
	for (unsigned int t = 0; t < MAX_THREAD; ++t)
	{
		threadPool[t] = std::thread(threadTest, std::ref(cellInfo));
	}
	for (unsigned int t = 0; t < MAX_THREAD; ++t)
	{
		threadPool[t].join();
	}
	i = cellInfo.best_i;
	j = cellInfo.best_j;
	return cellInfo.bestScore;
#else
	double score = -1;
	best_i = 0;
	best_j = 0;

	for (unsigned int i = 0; i < NSIDE; ++i)
		for (unsigned int j = 0; j < NSIDE; ++j)
		{
			double temp = compare(matrix_[i][j]->getLowPatch(), p);
			if (temp > score) {
				score = temp;
				best_i = i;
				best_j = j;
			}
		}
	return score;
#endif
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
double const SOM<NSIDE,LSIDE,HSIDE>::train(Cell<LSIDE,HSIDE> &cell, unsigned int *best_i, unsigned int *best_j)
{
	const double nradius = neighborhoodRadius();
	const double b = -(nradius * nradius) / log(0.001);
	const double alpha = learnRate();

	unsigned int new_i = 0, new_j = 0;
	double score = getBest(new_i, new_j, cell.getLowPatch());
	for (unsigned int i = 0; i < NSIDE; ++i)
		for (unsigned int j = 0; j < NSIDE; ++j)
		{
			const double factor = distanceFactor(i, j, new_i, new_j, b);
			matrix_[i][j]->setCell(cell, alpha*factor);
		}
	iter_++;
	if(best_i!=NULL)
		*best_i = new_i;
	if (best_j != NULL)
		*best_j = new_j;
	return score;
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
void SOM<NSIDE, LSIDE, HSIDE>::test(const Patch<LSIDE> &lrPatch, Patch<HSIDE> &hrPatch, Patch<HSIDE> *scores) const
{
	const double avg = lrPatch.getAvg();
	const double dev = lrPatch.getDev() * 1.25; //1.415
	hrPatch.zeroInitialize();
	if(scores != NULL)
		scores->zeroInitialize();

	// Test current patch
	for (unsigned int i = 0; i < NSIDE; ++i)
		for (unsigned int j = 0; j < NSIDE; ++j)
		{
			const double score = compare(matrix_[i][j]->getLowPatch(), lrPatch);

			const double **patchHI = (const double**)matrix_[i][j]->getHighPatch().getImage();

			for (unsigned int yy = 0; yy < HSIDE; ++yy)
			{
				for (unsigned int xx = 0; xx < HSIDE; ++xx)
				{
#if 0 // 0=No gaussian composition   -1=Gaussian composition
					const double scoreTemp = score * Patch<HSIDE>::getGauss()->getImage()[yy][xx];
#else
					const double scoreTemp = score;
#endif

					hrPatch.getImage()[yy][xx] += (patchHI[yy][xx] * dev + avg) * scoreTemp;		
					if(scores!=NULL)
						scores->getImage()[yy][xx] += scoreTemp;
				}
			}			
		}
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
void SOM<NSIDE,LSIDE,HSIDE>::exportSOMImage(const char* path, double avg, double dev ) const
{
	const unsigned int lowSomSide = LSIDE * NSIDE;
	const unsigned int hiSomSide = HSIDE * NSIDE;

	Image lrImg = Image(lowSomSide,lowSomSide);
	Image hrImg = Image(hiSomSide,hiSomSide);

	for(unsigned int i = 0; i< lowSomSide; i += LSIDE)
		for (unsigned int j = 0; j < lowSomSide; j += LSIDE)
			matrix_[i/LSIDE][j/LSIDE]->writeCell(lrImg, i, j, hrImg, 2 * i, 2 * j, avg, dev);
		
	char pathLR[2048];
	sprintf(pathLR, "%s%s", path, "-LR.pgm");
	lrImg.exportImagePGM(pathLR);

	char pathHR[2048];
	sprintf(pathHR, "%s%s", path, "-HR.pgm");
	hrImg.exportImagePGM(pathHR);
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
double const SOM<NSIDE,LSIDE,HSIDE>::compare(const Patch<LSIDE> &p1, const Patch<LSIDE> &p2) const
{
	static const double K = -0.01 / (log(0.0001)); // -0.01/(log (0,0001) )
	double dist = 0.0;

	double **const img1 = p1.getImage();
	double **const img2 = p2.getImage();
	for (unsigned int i = 0; i < LSIDE; i++)
	{
		const double *row1 = img1[i];
		const double *row2 = img2[i];
		for (unsigned int j = 0; j < LSIDE; j++)
		{
			double temp = row1[j] - row2[j];
			temp *= temp;
			dist += temp;
		}
	}
	dist /= p1.getSize();
	return exp(-dist / K);
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
double const SOM<NSIDE,LSIDE,HSIDE>::distanceFactor(const unsigned int i,const unsigned int j,const unsigned int best_i,const unsigned int best_j,const double b) const
{
	int tmp = abs((int)i - (int)best_i);
	int min_i = (int)NSIDE - tmp;
	if (tmp < min_i) min_i = tmp;
	min_i *= min_i;

	tmp = abs((int)j - (int)best_j);
	int min_j = (int)NSIDE - tmp;
	if (tmp < min_j) min_j = tmp;
	min_j *= min_j;

	return exp(-(min_i + min_j) / b);
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
double const SOM<NSIDE,LSIDE,HSIDE>::learnRate() const
{
	static const unsigned long long MAX_ITER = 1000000;
	static const double startRate = 0.005;
	static const double finalRate = 0.0000001;

	static const double range = finalRate - startRate;
	double rate = finalRate;
	if (iter_ < MAX_ITER)
		rate = startRate + ((double)iter_) / ((double)MAX_ITER)*range;
	return rate;
}

template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
double const SOM<NSIDE,LSIDE,HSIDE>::neighborhoodRadius() const
{
	static const unsigned long long MAX_ITER = 100000;
	static const double startRadius = NSIDE;
	static const double finalRadius = 1;

	static const double range = finalRadius - startRadius;
	double radius = finalRadius;
	if (iter_ < MAX_ITER) {
		radius = startRadius + ((double)iter_) / ((double)MAX_ITER)*range;
	}
	return radius;
}
