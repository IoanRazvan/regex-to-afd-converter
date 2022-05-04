#include "RegExToDFAConverter.h"

int main()
{
    RegExToDFAConverter converter("input.txt");
    converter.parseFile();
    converter.buildAFD();
    converter.writeDOTFile("afd.gv");
    return 0;
}