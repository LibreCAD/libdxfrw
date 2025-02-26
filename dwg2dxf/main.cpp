/******************************************************************************
**  dwg2dxf - Program to convert dwg/dxf to dxf(ascii & binary)              **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "dx_iface.h"
#include "dx_data.h"

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

void usage(){
    std::cout << "Usage: " << std::endl;
    std::cout << "   dwg2dxf <input> [-b] <-version> <output>" << std::endl << std::endl;
    std::cout << "   input      existing file to convert" << std::endl;
    std::cout << "   -b         optional, sets output as binary dxf" << std::endl;
    std::cout << "   -B         optional, batch mode reads a text file whit a list of full path input" << std::endl;
    std::cout << "               files and saves with the same name in the indicated folder as output" << std::endl;
    std::cout << "   -y -Y      optional, Warning! if output dxf exist overwrite without ask" << std::endl;
    std::cout << "   -version   version output of dxf file" << std::endl;
    std::cout << "   output     output file name" << std::endl << std::endl;
    std::cout << "     version can be:" << std::endl;
    std::cout << "        -R12   dxf release 12 version" << std::endl;
    std::cout << "        -v2000 dxf version 2000" << std::endl;
    std::cout << "        -v2004 dxf version 2004" << std::endl;
    std::cout << "        -v2007 dxf version 2007" << std::endl;
    std::cout << "        -v2010 dxf version 2010" << std::endl;
}

DRW::Version checkVersion(std::string param){
    if (param == "-R12")
        return DRW::AC1009;
    else if (param == "-v2000")
        return DRW::AC1015;
    else if (param == "-v2004")
        return DRW::AC1018;
    else if (param == "-v2007")
        return DRW::AC1021;
    else if (param == "-v2010")
        return DRW::AC1024;
    return DRW::UNKNOWNV;
}

bool convertFile(std::string inName, std::string outName, DRW::Version ver, bool binary, bool overwrite, bool debug){
    bool badState = false;
    //verify if input file exist
    std::ifstream ifs;
    ifs.open (inName.c_str(), std::ifstream::in);
    badState = ifs.fail();
    ifs.close();
    if (badState) {
        std::cout << "Error can't open " << inName << std::endl;
        return false;
    }
    //verify if output file exist
    std::ifstream ofs;
    ofs.open (outName.c_str(), std::ifstream::in);
    badState = ofs.fail();
    ofs.close();
    if (!badState) {
        if (!overwrite){
            std::cout << "File " << outName << " already exist, overwrite Y/N ?" << std::endl;
            int c = getchar();
            if (! ('y' == c || 'Y' == c)) {
                std::cout << "Cancelled.";
                return false;
            }
        }
    }
    //All ok proceed whit conversion
    //class to store file read:
    dx_data fData;
    //First read a dwg or dxf file
    dx_iface *input = new dx_iface();
    badState = input->fileImport( inName, &fData, debug);
    if (!badState) {
        std::cout << "Error reading file " << inName << std::endl;
        return false;
    }

    //And write a dxf file
    dx_iface *output = new dx_iface();
    std::string temp = output->fileExport(ver, binary, &fData, debug);

    std::ofstream file;
    if (binary) {
        file.open (outName, std::ios_base::out | std::ios::binary | std::ios::trunc);
    } else {
        file.open (outName, std::ios_base::out | std::ios::trunc);
    }
    if (!file) {
        std::cerr << "Failed to open file for writing: " << outName << std::endl;
        return false;
    }
    // Write the content of the string to the file
    file.write(temp.c_str(), temp.size());

    if (!file) {
        std::cerr << "Error occurred while writing to file: " << outName << std::endl;
        return false;
    }
    file.close();

    delete input;
    delete output;

    return badState;
}

int main(int argc, char *argv[]) {
    bool badState = false;
    bool binary = false;
    bool overwrite = false;
    bool batch = false;
    bool debug = false;
    std::string outName;
    DRW::Version ver = DRW::UNKNOWNV;
    if (argc < 3) {
        usage();
        return 1;
    }

//parse params.
    std::string fileName = argv[1];
    for (int i= 2; i < argc; ++i){
        std::string param = argv[i];
        if (i == (argc - 1) )
            outName = param;
        else {
            if (param.at(0) == '-') {
                if (param.at(1) == 'b')
                    binary = true;
                else if (param.at(1) == 'y')
                    overwrite = true;
                else if (param.at(1) == 'B')
                    batch = true;
                else if (tolower(param.at(1)) == 'd')
                    debug = true;
                else {
                    ver = checkVersion(param);
                    if (ver == DRW::UNKNOWNV) {
                        badState = true;
                    }
                }
            } else
                badState = true;
        }
    }

    if (badState) {
        std::cout << "Bad options." << std::endl;
        usage();
        return 1;
    }

    if (!batch){ //no batch mode, only one file
        bool ok = convertFile(fileName, outName, ver, binary, overwrite, debug);
        if (ok) {
            std::cout << "Success: converted " << fileName << " to " << outName << std::endl;
            return 0;
        }
        else {
            std::cout << "\nConversion failed" << std::endl;
            return 1;
        }

    }

 //batch mode, prepare input file and output folder.
    //verify if input file exist
    std::ifstream ifs;
    ifs.open (fileName.c_str(), std::ifstream::in);
    badState = ifs.fail();
    ifs.close();
    if (badState) {
        std::cout << "Batch mode, Error can't open " << fileName << std::endl;
        return 2;
    }

    //verify existence of output directory
    struct stat statBuf;
    int dirStat = stat(outName.c_str(), &statBuf);
    if(dirStat != 0 || S_ISDIR(statBuf.st_mode) == 0) {
        std::cout << "Batch mode: " << outName << " must be an existing directory" << std::endl;
        usage();
        return 4;
    }
    outName+="/";
    //create a list with the files to convert.
    std::ifstream bfs;
    bfs.open (fileName.c_str(), std::ifstream::in);
    std::list<std::string>inList;
    std::string line;
    while ( bfs.good() ){
        std::getline(bfs, line);
        if(!line.empty())
            inList.push_back(line);
    }
    for (std::list<std::string>::const_iterator it=inList.begin(); it!=inList.end(); ++it){
        std::string input = *it;
        unsigned found = input.find_last_of("/\\");
        std::string output = outName + input.substr(found+1);
        std::cout << "Converting file " << input << " to " << output << std::endl;
        bool ok = convertFile(input, output, ver, binary, overwrite, debug);
        if (!ok)
            std::cout << "Failed" << std::endl;
    }

    return 0;
}

