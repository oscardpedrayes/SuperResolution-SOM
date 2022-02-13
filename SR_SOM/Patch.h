#ifndef __PATCH_H__
#define __PATCH_H__

#include "Image.h"
template <unsigned int PSIDE>
class Patch
{
public:
	//Creates a Patch with side s
	Patch();

	~Patch();

	void randomInitialize(double sigma);
	void zeroInitialize();

	// Getters
	double** getImage() const;
	unsigned int const getSide() const;
	unsigned int const getSize() const;
	double getAvg() const;
	double getDev() const;
	static const Patch<PSIDE>* getGauss();

	void setAvg(const double avg) ;
	void setDev(const double dev) ;

	// Mixes with the specified Patch by a factor
	void setPatch(const Patch<PSIDE> &p, const double factor);

	// Substracts a Patch to the local one
	void diffPatch(const Patch<PSIDE> &p);

	// Adds a value to a position of the image
	void addValue(const double val, const unsigned int i, const unsigned int j);

	// Adds a value to every position of the image
	void addAllValue(const double val);
	// Multiply a value to every position of the image
	void mulAllBy(const double val);

	// Reads a Patch from a Image
	void readPatch(const Image &image, const unsigned int i, const unsigned int j);
	void calculateAVG_DEV();
	// Writes a Patch into a Image
	void writePatch(Image &image, const unsigned int i, const unsigned int j) const;
	void addPatch(Image &image, const unsigned int i, const unsigned int j) const;
	void writePatch(Image &image, const unsigned int i, const unsigned int j, const double avg, const double dev) const;


protected:
	static void initGaussian();

protected:

	double **imagePatch_;
	unsigned int sizePatch_;
	double avg_;
	double dev_;

	static Patch<PSIDE> *__gauss;
};
#include "Patch.cc"
#endif
