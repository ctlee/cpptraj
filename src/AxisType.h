#ifndef INC_AXISTYPE_H
#define INC_AXISTYPE_H
#include "Frame.h"
#include "Name.h"
#ifdef NASTRUCTDEBUG
#  include "CpptrajFile.h"
#endif
/*! \file AxisType.h
    \brief Hold classes and functions used for NA structure analysis.
 */
/// Declared outside class since it is used by AxisType and NAstruct
enum NAbaseType { UNKNOWN_BASE, DA, DT, DG, DC, RA, RC, RG, RU };
NAbaseType ID_base(char*);
// Class: AxisType
/// Frame for NA bases, intended for use with NAstruct Action.
/** AxisType is a special kind of Frame. It will be used in 2 cases: 
  * - To hold the coordinates of reference bases for RMS fitting onto input
  *   coordinates in order to obtain reference frames.
  * - For holding the coordinates of the origin and XYZ axes of the 
  *   reference frames themselves.
  */
class AxisType : public Frame {
    static const char NAbaseName[][5];
    NAME *Name;  ///< Atom/Axis names
    int maxAtom; ///< Actual size of memory. natom may be less than this.

    int AllocAxis(int);
  public:
    NAbaseType ID;
    double R[9];

    AxisType();
    AxisType(int);
    ~AxisType();
    void RX(double Vin[3]);
    void RY(double Vin[3]);
    void RZ(double Vin[3]);
    double *Origin();
    char *BaseName();
    //int AtomIndex(char *);
    bool AtomNameIs(int, char *);
    char *AtomName(int);
    void SetFromFrame(AxisType *);
    void StoreRotMatrix(double*);
    void SetPrincipalAxes();
    int SetRefCoord(char *);
    void FlipYZ();
    void FlipXY();
#ifdef NASTRUCTDEBUG
    void WritePDB(CpptrajFile *, int, char *, int *); // DEBUG
#endif
};
#endif  
