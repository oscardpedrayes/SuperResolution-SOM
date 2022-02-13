#include "Cell.h"

template <unsigned int LSIDE, unsigned int HSIDE>
Cell<LSIDE, HSIDE>::Cell(double sigma)
	: hiPatch_()
	, loPatch_()
{
	loPatch_.randomInitialize(sigma);
	hiPatch_.randomInitialize(sigma);
}

template <unsigned int LSIDE, unsigned int HSIDE>
Cell<LSIDE, HSIDE>::~Cell()
{		
}

template <unsigned int LSIDE, unsigned int HSIDE>
Patch<HSIDE>& Cell<LSIDE, HSIDE>::getHighPatch()
{
	return hiPatch_;
}

template <unsigned int LSIDE, unsigned int HSIDE>
Patch<LSIDE>& Cell<LSIDE, HSIDE>::getLowPatch()
{
	return loPatch_;
}

template <unsigned int LSIDE, unsigned int HSIDE>
void Cell<LSIDE, HSIDE>::setCell(const Cell<LSIDE,HSIDE> &cell, const double factor)
{
	hiPatch_.setPatch(cell.hiPatch_, factor);
	loPatch_.setPatch(cell.loPatch_, factor);
}

template <unsigned int LSIDE, unsigned int HSIDE>
void Cell<LSIDE, HSIDE>::diffCell(const Cell<LSIDE,HSIDE> &cell)
{
	hiPatch_.diffPatch(cell.hiPatch_);
	loPatch_.diffPatch(cell.loPatch_);
}

template <unsigned int LSIDE, unsigned int HSIDE>
void Cell<LSIDE, HSIDE>::readCell(const Image &imgLow, const unsigned int iLow, const unsigned int jLow, const Image &imgHi, const unsigned int iHi, const unsigned int jHi)
{
	loPatch_.readPatch(imgLow, iLow, jLow);
	hiPatch_.readPatch(imgHi, iHi, jHi);
}

template <unsigned int LSIDE, unsigned int HSIDE>
void Cell<LSIDE, HSIDE>::writeCell(Image &imgLow,const unsigned int iLow,const unsigned int jLow, Image &imgHi,const unsigned int iHi,const unsigned int jHi) const
{
	loPatch_.writePatch(imgLow, iLow, jLow);
	hiPatch_.writePatch(imgHi, iHi, jHi);
}

template <unsigned int LSIDE, unsigned int HSIDE>
void Cell<LSIDE, HSIDE>::writeCell(Image &imgLow, const unsigned int iLow, const unsigned int jLow, Image &imgHi, const unsigned int iHi, const unsigned int jHi, const double avg, const double dev) const
{
	loPatch_.writePatch(imgLow, iLow, jLow, avg, dev);
	hiPatch_.writePatch(imgHi, iHi, jHi, avg, dev);
}