#include "RegExToAFDConverter.h"

int main()
{
    RegExToAFDConverter converter("input.txt");
    converter.parseFile();
    converter.buildAFD();
    converter.writeDOTFile("afd.gv");
    return 0;
}