#ifndef _PixelFEDTestDAC_h_
#define _PixelFEDTestDAC_h_
/**
*   \file CalibFormats/SiPixelObjects/interface/PixelFEDTestDAC.h
*   \brief This class implements..
*
*   This class specifies which FED boards
*   are used and how they are addressed
*/

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include "CalibFormats/SiPixelObjects/interface/PixelCalibBase.h"

namespace pos{

/*! \ingroup CalibrationObjects
*
*   @{
*
*   \class PixelFEDTestDAC PixelFEDTestDAC.h "interface/PixelFEDTestDAC.h"
*   \brief This class implements..
*
*   A longer explanation will be placed here later
*/
  class PixelFEDTestDAC : public PixelCalibBase {

  public:
    PixelFEDTestDAC(std::string filename);
    PixelFEDTestDAC(std::vector< std::vector<std::string> > &);
    std::string mode() const override {return mode_;}
    std::vector<unsigned int> dacs() {return dacs_;}
    void writeXMLHeader(  pos::PixelConfigKey key, 
				  int version, 
				  std::string path, 
				  std::ofstream *out,
				  std::ofstream *out1 = nullptr,
				  std::ofstream *out2 = nullptr
				  ) const override ;
    void writeXML( 	  std::ofstream *out,		        				    
			   	  std::ofstream *out1 = nullptr ,	       
			   	  std::ofstream *out2 = nullptr ) const override ; 
    void writeXMLTrailer( std::ofstream *out, 
				  std::ofstream *out1 = nullptr,
				  std::ofstream *out2 = nullptr
				  ) const override ;

  private:

    unsigned int levelEncoder(int level);
    std::vector<unsigned int> decimalToBaseX(unsigned int a, unsigned int x, unsigned int length);
    std::vector<unsigned int> dacs_;

  };
}
/* @} */
#endif




	
	
