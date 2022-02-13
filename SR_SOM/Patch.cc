#include <fstream>
#include <stdlib.h>
#include "Patch.h"

template <unsigned int PSIDE>
Patch<PSIDE> *Patch<PSIDE>::__gauss = NULL;

template <unsigned int PSIDE>
Patch<PSIDE>::Patch()
	: imagePatch_(nullptr)
	, sizePatch_(PSIDE*PSIDE)
	,avg_(0)
	,dev_(1)
{
	if (PSIDE > 0)
	{
		imagePatch_ = new double*[PSIDE];
		for (unsigned int i = 0; i < PSIDE; i++)
		{
			imagePatch_[i] = new double[PSIDE];			
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::initGaussian()
{
	const double sideMid = 0.5 * (PSIDE - 1);
	const double K = log(0.25) / (2 * sideMid * sideMid);
	__gauss = new Patch<PSIDE>();

	double sum = 0;
	for (unsigned int i = 0; i < PSIDE; ++i)
		for (unsigned int j = 0; j < PSIDE; ++j)
		{
			const double dist_i = i - sideMid;
			const double dist_j = j - sideMid;
			__gauss->imagePatch_[i][j] = exp((dist_i * dist_i + dist_j * dist_j) * K);
			sum += __gauss->imagePatch_[i][j];
		}
	for (unsigned int i = 0; i < PSIDE; ++i)
		for (unsigned int j = 0; j < PSIDE; ++j)
			__gauss->imagePatch_[i][j] /= sum;
}

template <unsigned int PSIDE>
Patch<PSIDE> const *Patch<PSIDE>::getGauss()
{
	return __gauss;
}

template <unsigned int PSIDE>
void Patch<PSIDE>::randomInitialize(double sigma) 
{	
	for (unsigned int i = 0; i < PSIDE; i++)
	{
		for (unsigned int j = 0; j < PSIDE; j++)
		{
			const double rand01 = ((double)rand()) / (double)RAND_MAX; //Random entre 0 y 1
			imagePatch_[i][j] = sigma * (2 * rand01 - 1); //Random entre -0.01 y 0.01
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::zeroInitialize()
{
	for (unsigned int i = 0; i < PSIDE; i++)
	{
		for (unsigned int j = 0; j < PSIDE; j++)
		{
			imagePatch_[i][j] = 0;
		}
	}
}

template <unsigned int PSIDE>
Patch<PSIDE>::~Patch()
{
	if (imagePatch_ != nullptr)
	{
		for (unsigned int i = 0; i < PSIDE; ++i)
			delete[] imagePatch_[i];
		delete[] imagePatch_;
	}
}

template <unsigned int PSIDE>
double** Patch<PSIDE>::getImage() const
{
	if (__gauss == NULL)
		initGaussian();
	return imagePatch_;
}
template <unsigned int PSIDE>
unsigned int const Patch<PSIDE>::getSide() const
{
	return PSIDE;
}

template <unsigned int PSIDE>
unsigned int const Patch<PSIDE>::getSize() const
{
	return sizePatch_;
}

template <unsigned int PSIDE>
double Patch<PSIDE>::getAvg() const
{
	return avg_;
}

template <unsigned int PSIDE>
double Patch<PSIDE>::getDev() const
{
	return dev_;
}

template <unsigned int PSIDE>
void Patch<PSIDE>::setAvg(const double avg)
{
	avg_ = avg;;
}

template <unsigned int PSIDE>
void Patch<PSIDE>::setDev(const double dev)
{
	dev_=dev;
}

template <unsigned int PSIDE>
void Patch<PSIDE>::readPatch(const Image &img,const unsigned int i, const unsigned int j)
{
	if (__gauss == NULL)
		initGaussian();

	avg_ = 0;
	dev_ = 0;
#if 0 // 0 => Media sin gauss -1 => Media con Gauss
	for (unsigned int k = 0; k < PSIDE; ++k)
	{
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			const double value = img.getImage()[i + k][j + l];
			avg_ += value * __gauss->imagePatch_[k][l];
			dev_ += value * value * __gauss->imagePatch_[k][l];
			imagePatch_[k][l] = value;
		}
	}
	dev_ -= avg_ * avg_;
#else
	for (unsigned int k = 0; k < PSIDE; ++k)
	{
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			const double value = img.getImage()[i + k][j + l];
			avg_ += value;
			dev_ += value * value;
			imagePatch_[k][l] = value;
		}
	}
	avg_ /= sizePatch_;
	dev_ = (dev_ / sizePatch_) - avg_ * avg_;
#endif
	if (dev_ < 0) dev_ = 0; // round error 
	dev_ = sqrt(dev_);
	if (dev_ == 0) dev_ = 1; // division by 0 fix


	for (unsigned int k = 0; k < PSIDE; ++k)
	{
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			imagePatch_[k][l] -= avg_;
			imagePatch_[k][l] /= dev_;
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::calculateAVG_DEV()
{
	if (__gauss == NULL)
		initGaussian();
	avg_ = 0;
	dev_ = 0;
#if 0 // 0 => Media sin gauss -1 => Media con Gauss
	for (unsigned int k = 0; k < PSIDE; ++k)
	{
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			const double value = imagePatch_[k][l];
			avg_ += value * __gauss->imagePatch_[k][l];
			dev_ += value * value * __gauss->imagePatch_[k][l];
			imagePatch_[k][l] = value;
		}
	}

	dev_ -=  avg_ * avg_;
#else
	for (unsigned int k = 0; k < PSIDE; ++k)
	{
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			const double value = imagePatch_[k][l];
			avg_ += value;
			dev_ += value * value;
			imagePatch_[k][l] = value;
		}
	}
	avg_ /= sizePatch_;
	dev_ = (dev_ / sizePatch_) - avg_ * avg_;
#endif
	if (dev_ < 0) dev_ = 0; // round error 
	dev_ = sqrt(dev_);
	if (dev_ == 0) dev_ = 1; // division by 0 fix
	for (unsigned int k = 0; k < PSIDE; ++k)
	{
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			imagePatch_[k][l] -= avg_;
			imagePatch_[k][l] /= dev_;
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::writePatch(Image &image,const unsigned int i,const unsigned int j) const
{
	for (unsigned int k = 0; k < PSIDE; ++k)
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			image.setValue(imagePatch_[k][l]*dev_ + avg_, i + k, j + l);
		}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::addPatch(Image &image, const unsigned int i, const unsigned int j) const
{
	for (unsigned int k = 0; k < PSIDE; ++k)
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			image.addValue(imagePatch_[k][l] * dev_ + avg_, i + k, j + l);
		}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::writePatch(Image &image, const unsigned int i, const unsigned int j, const double avg, const double dev) const
{
	for (unsigned int k = 0; k < PSIDE; ++k)
		for (unsigned int l = 0; l < PSIDE; ++l)
		{
			image.setValue(imagePatch_[k][l] * dev + avg, i + k, j + l);
		}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::diffPatch(const Patch<PSIDE> &patch)
{
	for (unsigned int i = 0; i < PSIDE; ++i)
	{
		for (unsigned int j = 0; j < PSIDE; ++j)
		{
			imagePatch_[i][j] -= patch.imagePatch_[i][j];
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::addAllValue(const double val)
{
	for (unsigned int i = 0; i < PSIDE; ++i)
	{
		for (unsigned int j = 0; j < PSIDE; ++j)
		{
			imagePatch_[i][j] += val;
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::mulAllBy(const double val)
{
	for (unsigned int i = 0; i < PSIDE; ++i)
	{
		for (unsigned int j = 0; j < PSIDE; ++j)
		{
			imagePatch_[i][j] *= val;
		}
	}
}

template <unsigned int PSIDE>
void Patch<PSIDE>::addValue(const double val,const unsigned int i,const unsigned int j)
{
	imagePatch_[i][j] += val;
}

template <unsigned int PSIDE>
void Patch<PSIDE>::setPatch(const Patch<PSIDE> &p, const double factor)
{
	for (unsigned int i = 0; i < PSIDE; i++) 
	{
		for (unsigned int j = 0; j < PSIDE; j++) 
		{
			imagePatch_[i][j] += factor * (p.imagePatch_[i][j] - imagePatch_[i][j]);
		}
	}

}

