//*******************************************************************
//
// License:  See top level LICENSE.txt file.
// 
// Author: Garrett Potts
//
// Description: 
//
//*******************************************************************
//  $Id: ossimMultiBandHistogram.cpp 21631 2012-09-06 18:10:55Z dburken $

#include <ossim/base/ossimMultiBandHistogram.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/imaging/ossimImageSource.h>
#include <fstream>

using namespace std;

ossimMultiBandHistogram::ossimMultiBandHistogram()
{  
}

ossimMultiBandHistogram::~ossimMultiBandHistogram()
{
   deleteHistograms();
}

ossimMultiBandHistogram::ossimMultiBandHistogram(const ossimMultiBandHistogram& rhs)
{
   theHistogramList.resize(rhs.theHistogramList.size());

   for(ossim_uint32 i = 0; i < theHistogramList.size(); ++i)
   {
      theHistogramList[i] =  rhs.theHistogramList[i].valid()?
         new ossimHistogram(*rhs.theHistogramList[i].get()):(ossimHistogram*)0;
   }
}

ossimMultiBandHistogram::ossimMultiBandHistogram(ossim_int32 numberOfBands,
                                                 ossim_int32 numberOfBuckets,
                                                 float minValue,
                                                 float maxValue,
                                                 float nullValue,
                                                 ossimScalarType scalar)
{
   if(numberOfBands > 0)
   {
      create(numberOfBands, numberOfBuckets, minValue, maxValue, nullValue, scalar);
   }
}

void ossimMultiBandHistogram::create(const ossimImageSource* input)
{
   if (input)
   {
      ossim_uint32 numberOfBins = 0;
      ossim_float32 minValue = 0.0;
      ossim_float32 maxValue = 0.0;
      ossim_float32 nullValue = 0.0;

      if ( ossim::getBinInformation( input, 0, numberOfBins,
                                     minValue, maxValue, nullValue ) )
      {
         create( input->getNumberOfOutputBands(),
                 numberOfBins, minValue, maxValue, nullValue,
                 input->getOutputScalarType() );
      }
   }
}

void ossimMultiBandHistogram::create(ossim_int32 numberOfBands,
                                     ossim_int32 numberOfBuckets,
                                     float minValue,
                                     float maxValue,
                                     float nullValue,
                                     ossimScalarType scalar)
{
   // Make sure we clear our internal lists before we start.
   deleteHistograms();

   if(numberOfBands > 0)
   {
      numberOfBuckets = numberOfBuckets>0?numberOfBuckets:1;

      for(ossim_int32 bands = 0; bands < numberOfBands; ++bands)
      {
         theHistogramList.push_back( new ossimHistogram(numberOfBuckets,
                                                        minValue,
                                                        maxValue,
                                                        nullValue,
                                                        scalar) );
      }
   }
}

void ossimMultiBandHistogram::create(ossim_int32 numberOfBands)
{
   deleteHistograms();
   for(ossim_int32 bands = 0; bands < numberOfBands; ++bands)
   {
      theHistogramList.push_back(new ossimHistogram);
   }
}

ossim_uint32 ossimMultiBandHistogram::getNumberOfBands() const
{
   return (ossim_uint32)theHistogramList.size();
}

ossimRefPtr<ossimHistogram> ossimMultiBandHistogram::getHistogram(ossim_int32 band)
{
   if((band >=0) && (band < (ossim_int32)theHistogramList.size()))
   {
      return theHistogramList[band];
   }

   return NULL;
}

const ossimRefPtr<ossimHistogram> ossimMultiBandHistogram::getHistogram(ossim_int32 band)const
{
   if((band >=0) && (band < (ossim_int32)theHistogramList.size()))
   {
      return theHistogramList[band];
   }

   return 0;
}

void ossimMultiBandHistogram::setBinCount(double binNumber, double count)
{
   if(theHistogramList.size() > 0)
   {
      for(ossim_uint32 idx = 0; idx < theHistogramList.size(); ++idx)
      {
         if(theHistogramList[idx].valid())
         {
            theHistogramList[idx]->SetCount(binNumber, count);
         }
      }
   }   
}

ossimRefPtr<ossimMultiBandHistogram> ossimMultiBandHistogram::createAccumulationLessThanEqual()const
{
   ossimRefPtr<ossimMultiBandHistogram> result = NULL;

   if(theHistogramList.size() > 0)
   {
      result = new ossimMultiBandHistogram;
      result->theHistogramList.resize(theHistogramList.size());
      
      for(ossim_uint32 idx = 0; idx < theHistogramList.size(); ++idx)
      {
         if(theHistogramList[idx].valid())
         {
            result->theHistogramList[idx] = theHistogramList[idx]->CumulativeLessThanEqual();
         }
         else
         {
            result->theHistogramList[idx] = 0;
         }     
      }
   }

   return result;
}

ossimRefPtr<ossimMultiBandHistogram> ossimMultiBandHistogram::createAccumulationGreaterThanEqual()const
{
   ossimRefPtr<ossimMultiBandHistogram> result = NULL;

   if(theHistogramList.size() > 0)
   {
      result = new ossimMultiBandHistogram;
      
      for(ossim_uint32 idx = 0; idx < theHistogramList.size(); ++idx)
      {
         if(theHistogramList[idx].valid())
         {
            result->theHistogramList[idx] = theHistogramList[idx]->CumulativeGreaterThanEqual();
         }
         else
         {
            result->theHistogramList[idx] = 0;
         }     
      }
   }

   return result;
}

void ossimMultiBandHistogram::deleteHistograms()
{
   theHistogramList.clear();
}


bool ossimMultiBandHistogram::importHistogram(std::istream& in)
{
   ossimProprietaryHeaderInformation header;
   deleteHistograms();
   
   if(header.parseStream(in))
   {
      ossim_int32 numberOfBands = header.getNumberOfBands();
      
      if(numberOfBands)
      {
         theHistogramList.resize(numberOfBands);

         for(ossim_int32 counter = 0; counter < (ossim_int32)theHistogramList.size(); ++counter)
         {
            theHistogramList[counter] = 0;
         }
         ossimString bandBuffer;
         ossimString buffer;
         
         for(ossim_int32 idx = 0; idx < numberOfBands; ++idx)
         {
            getline(in, buffer);
            if(buffer.find("Band") != string::npos)
            {
				string::size_type offset = buffer.find(":");
               if(offset != string::npos)
               {
                  bandBuffer = buffer.substr(offset+1);
               }
               else
               {
                  deleteHistograms();
                  return false;
               }
            }
            else
            {
               deleteHistograms();
               return false;
            }
            ossim_uint32 bandIdx = bandBuffer.toUInt32();

            if(bandIdx < theHistogramList.size())
            {
               if(!theHistogramList[bandIdx].valid())
               {
                  ossimRefPtr<ossimHistogram> histogram = new ossimHistogram;
                  if(histogram->importHistogram(in))
                  {
                     theHistogramList[bandIdx] = histogram;
                  }
                  else
                  {
                     deleteHistograms();
                     return false;
                  }
               }
            }
            else
            {
               deleteHistograms();
               return false;
            }
         }
      }
      else
      {
         return false;
      }
   }  
   return true;
}

bool ossimMultiBandHistogram::importHistogram(const ossimFilename& file)
{
   if(file.exists())
   {
      std::ifstream input(file.c_str());

      return importHistogram(input);
   }

   return false;
}

bool ossimMultiBandHistogram::ossimProprietaryHeaderInformation::parseStream(std::istream& in)
{
   ossimString inputLine;

   getline(in, inputLine);  
   if(inputLine.find("File Type") != string::npos)
   {
      std::string::size_type idx = inputLine.find(":");
      if(idx != string::npos)
      {
         theFileType = inputLine.substr(idx+1);
         theFileType = theFileType.trim();
      }
      else
      {
         return false;
      }

   }
   else
   {
      return false;
   }

   getline(in, inputLine);  
   if(inputLine.find("Version") != string::npos)
   {
      std::string::size_type idx = inputLine.find(":");
      if(idx != string::npos)
      {
         theVersion = inputLine.substr(idx+1);
         theVersion = theVersion.trim();
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }

   getline(in, inputLine);  
   if(inputLine.find("Number of Bands") != string::npos)
   {
      std::string::size_type idx = inputLine.find(":");
      if(idx != string::npos)
      {
         theNumberOfBands = inputLine.substr(idx+1);
         theNumberOfBands = theNumberOfBands.trim();
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
   
   return true;
}

ossim_uint32 ossimMultiBandHistogram::ossimProprietaryHeaderInformation::getNumberOfBands() const
{
   return theNumberOfBands.toUInt32();
}

void ossimMultiBandHistogram::ossimProprietaryHeaderInformation::clear()
{
   theFileType      = "";
   theVersion       = "";
   theNumberOfBands = "";
}

bool ossimMultiBandHistogram::saveState(ossimKeywordlist& kwl,
                                        const char* prefix)const
{
   kwl.add(prefix,
           "type",
           "ossimMultiBandHistogram",
           true);
   kwl.add(prefix,
           "number_of_bands",
           static_cast<ossim_uint32>(theHistogramList.size()),
           true);
   
   for(ossim_uint32 idx = 0; idx < theHistogramList.size(); ++idx)
   {
      ossimString band = ossimString(prefix) + "band";
      band += (ossimString::toString(idx) + ".");
      if(theHistogramList[idx].valid())
      {
         ossimString newPrefix = (ossimString(prefix) + ossimString::toString(idx) + ".");
         
         theHistogramList[idx]->saveState(kwl, band.c_str());
      }
   }
   
   return true;
}

bool ossimMultiBandHistogram::loadState(const ossimKeywordlist& kwl,
                                        const char* prefix)
{
   deleteHistograms();
   const char* number_of_bands = kwl.find(prefix, "number_of_bands");
   if(number_of_bands)
   {
      ossim_uint32 numberOfBands = ossimString(number_of_bands).toUInt32();

      if(numberOfBands>0)
      {
         ossimString newPrefix;
         for(ossim_uint32 idx = 0; idx < numberOfBands; ++idx)
         {
            ossimHistogram* histo = new ossimHistogram;

            newPrefix = prefix;
            newPrefix += "band";
            newPrefix += ossimString::toString(idx);
            newPrefix += ".";
            
            histo->loadState(kwl, newPrefix.c_str());
            
            theHistogramList.push_back(histo);
         }
      }
   }
   
   return true;
}

bool ossimMultiBandHistogram::saveState(ossimRefPtr<ossimXmlNode> xmlNode)const
{
   xmlNode->setTag("ossimMutliBandHistogram");
   for(ossim_uint32 idx = 0; idx < theHistogramList.size(); ++idx)
   {
      ossimRefPtr<ossimXmlNode> band = new ossimXmlNode;
      ossimRefPtr<ossimXmlNode> newNode = new ossimXmlNode;

      band->setTag("Band");
      band->addAttribute("idx", ossimString::toString(idx));
      band->addChildNode(newNode.get());
      if(theHistogramList[idx].valid())
      {         
         theHistogramList[idx]->saveState(newNode);
      }
      else
      {
         newNode->setTag("ossimHistogram");
      }
      xmlNode->addChildNode(band.get());
   }

   return true;
}

bool ossimMultiBandHistogram::loadState(const ossimRefPtr<ossimXmlNode> xmlNode)
{
   theHistogramList.clear();
   const vector<ossimRefPtr<ossimXmlNode> >& childNodes = xmlNode->getChildNodes();

   ossim_uint32 idx = 0;
   ossim_uint32 maxCount = (ossim_uint32)childNodes.size();
   for(idx = 0; idx < maxCount; ++idx)
   {
      if(childNodes[idx]->getTag() == "Band")
      {
         if(childNodes[idx]->getChildNodes().size())
         {
            ossimRefPtr<ossimHistogram> histo = new ossimHistogram;
            histo->loadState(childNodes[idx]->getChildNodes()[0]);
            theHistogramList.push_back(histo);
         }
      }
   }

   return true;
}
