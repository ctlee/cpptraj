#include "Analysis.h"
#include "DataSet_double.h"
// Class: Corr
class Corr : public Analysis {
    DataSet *D1;
    DataSet *D2;
    int lagmax;
    int Nelements;
    DataSet_double Ct;
    char *outfilename;

  public:
    Corr();
    ~Corr();

    int Setup(DataSetList*);
    int Analyze();
    void Print(DataFileList*);
};
