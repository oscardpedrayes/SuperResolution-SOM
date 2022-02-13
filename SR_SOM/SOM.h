#ifndef __SOM_H__
#define __SOM_H__

#include "Patch.h"
#include "Cell.h"


// Matrix of Cells
template <unsigned int NSIDE, unsigned int LSIDE, unsigned int HSIDE>
class SOM 
{
public:
	// Creates a SOM by loading it from a file
	SOM(const char *path, const unsigned int stp);
	// Creates a new SOM
	SOM(const unsigned int stp, double sigma);
	~SOM();

	const unsigned long long getIter() const;

	// Saves current SOM on a file
	void save(FILE *file) const;
	// Loads SOM from a file
	void load(FILE *file);

	// Gets Patch with best coincidente
	double const getBest(unsigned int &best_i, unsigned int &best_j, const Patch<LSIDE> &p) const;

	// Tests a LR-patch
	void test(const Patch<LSIDE> &lrPatch, Patch<HSIDE> &hrPatch, Patch<HSIDE> *score=NULL) const;
	// Trains a cell
	double const train(Cell<LSIDE,HSIDE> &c, unsigned int *best_i=NULL, unsigned int *best_j=NULL);
	
	// Exports the SOM to the disk as a image
	void exportSOMImage(const char* path, double avg, double dev) const;
		
protected:	
	// Allocates space for the matrix
	void initializeMatrix(double sigma);
		

	// Returns de value of the 
	double const compare(const Patch<LSIDE> &p1,const Patch<LSIDE> &p2) const;
	// Returns the value of the distance between Cells
	double const distanceFactor(const unsigned int i,const unsigned int j,const unsigned int best_i,const unsigned int best_j,const double b) const;
	// Returns the learn rate 
	double const learnRate() const;
	// Returns the radius of action
	double const neighborhoodRadius() const;

protected:
	//Matrix of Cells to store the SOM
	Cell<LSIDE,HSIDE>* matrix_[NSIDE][NSIDE];

	// Number of iterations
	unsigned long long iter_;
	// Number of pixels that will use as a step in training
	unsigned int step_;
};

// This is a template, so it could not be a CPP file
#include "SOM.cc"

#endif