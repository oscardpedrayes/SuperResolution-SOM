#ifndef __CELL_H__
#define __CELL_H__

#include "Patch.h"
#include "Image.h"

// Contains a low-resolution and a high-resolution Patches
template <unsigned int LSIDE, unsigned int HSIDE>
class Cell
{
public:
	Cell(double sigma);
	~Cell();

	//Getters
	Patch<LSIDE>& getLowPatch();
	Patch<HSIDE>& getHighPatch();

	// Mixes the Patches of the Cell with another by a factor
	void setCell(const Cell<LSIDE,HSIDE> &c, const double factor);
	// Subtracts the specified cell
	void diffCell(const Cell<LSIDE,HSIDE> &c);
	// Reads both loPatch and hiPatch of a Image starting on the position specified
	void readCell(const Image &imgLow,const unsigned int iLow,const unsigned int jLow, const Image &imgHi,const unsigned int iHi,const unsigned int jHi);
	// Writes onto a Image the actual Patch starting on the position specified
	void writeCell(Image &imgLow,const unsigned int iLow,const unsigned int jLow, Image &imgHi,const unsigned int iHi,const unsigned int jHi) const;
	void writeCell(Image &imgLow, const unsigned int iLow, const unsigned int jLow, Image &imgHi, const unsigned int iHi, const unsigned int jHi, const double avg, const double dev) const;

protected:

	Patch<HSIDE> hiPatch_;
	Patch<LSIDE> loPatch_;
};

#include "Cell.cc"
#endif
