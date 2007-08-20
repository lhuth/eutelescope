// -*- mode: c++; mode: auto-fill; mode: flyspell-prog; -*-
// Author Antonio Bulgheroni, INFN <mailto:antonio.bulgheroni@gmail.com>
// Version $Id: EUTelRawDataSparsifier.cc,v 1.2 2007-08-20 16:48:38 bulgheroni Exp $
/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

// eutelescope includes ".h" 
#include "EUTELESCOPE.h"
#include "EUTelMatrixDecoder.h"
#include "EUTelSparseDataImpl.h"
#include "EUTelBaseSparsePixel.h"
#include "EUTelSimpleSparsePixel.h"
#include "EUTelExceptions.h"
#include "EUTelRawDataSparsifier.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelEventImpl.h"

// marlin includes ".h"
#include "marlin/Processor.h"
#include "marlin/Exceptions.h"

// lcio includes <.h> 
#include <IMPL/TrackerRawDataImpl.h>
#include <IMPL/TrackerDataImpl.h>
#include <IMPL/LCCollectionVec.h>
#include <UTIL/CellIDDecoder.h>
#include <UTIL/CellIDEncoder.h>

// system includes <>
#include <vector>
#include <memory>

using namespace std;
using namespace lcio;
using namespace marlin;
using namespace eutelescope;

EUTelRawDataSparsifier::EUTelRawDataSparsifier () :Processor("EUTelRawDataSparsifier") {

  // modify processor description
  _description =
    "EUTelRawDataSparsifier transform full frame raw data in ZS calibrated data mimicking the EUDRB behavior. ";

  // first of all we need to register the input collection
  registerInputCollection (LCIO::TRACKERRAWDATA, "RawDataCollectionName",
			   "Input raw data collection",
			   _rawDataCollectionName, string ("rawdata"));

  registerInputCollection (LCIO::TRACKERDATA, "PedestalCollectionName",
			   "Pedestal from the condition file",
			   _pedestalCollectionName, string ("pedestal"));

  registerInputCollection (LCIO::TRACKERDATA, "NoiseCollectionName",
			   "Noise from the condition file",
			   _noiseCollectionName, string("noise"));

  registerInputCollection (LCIO::TRACKERRAWDATA, "StatusCollectionName",
			   "Pixel status from the condition file",
			   _statusCollectionName, string("status"));

  registerOutputCollection (LCIO::TRACKERDATA, "SparsifiedDataCollectionName",
			    "Name of the output sparsified data collection",
			    _sparsifiedDataCollectionName, string("data"));

  registerProcessorParameter("SparsePixelType", "Type of sparsified pixel data structure (use SparsePixelType enum)",
			     _pixelType , static_cast<int> ( 1 ) );
  
  vector<float > sigmaCutVecExample;
  sigmaCutVecExample.push_back(2.5);
  sigmaCutVecExample.push_back(2.5);
  sigmaCutVecExample.push_back(2.5);
  sigmaCutVecExample.push_back(2.5);
  sigmaCutVecExample.push_back(2.5);

  registerProcessorParameter("SigmaCut","A vector of float containing for each plane the multiplication factor for the noise",
			     _sigmaCutVec, sigmaCutVecExample);
}


void EUTelRawDataSparsifier::init () {
  // this method is called only once even when the rewind is active
  // usually a good idea to
  printParameters ();

  // set to zero the run and event counters
  _iRun = 0;
  _iEvt = 0;

}

void EUTelRawDataSparsifier::processRunHeader (LCRunHeader * rdr) {

  // to make things easier re-cast the input header to the EUTelRunHeaderImpl
  EUTelRunHeaderImpl *  runHeader = static_cast<EUTelRunHeaderImpl*>(rdr);

  _noOfDetector = runHeader->getNoOfDetector();

  // let's check if the number of sigma cut components is the same of
  // the detector number.
  if ( (unsigned) _noOfDetector != _sigmaCutVec.size() ) {
    message<WARNING> ( log() << "The number of values in the sigma cut does not match the number of detectors\n"
		       << "Changing SigmaCutVec consequently." );
    _sigmaCutVec.resize(_noOfDetector, _sigmaCutVec.back());
  }

  // increment the run counter
  ++_iRun;

}


void EUTelRawDataSparsifier::processEvent (LCEvent * event) {

  EUTelEventImpl * evt = static_cast<EUTelEventImpl*> (event);

  if ( evt->getEventType() == kEORE ) {
    message<DEBUG> ( "EORE found: nothing else to do." );
    return;
  }  

  if (_iEvt % 10 == 0) 
    message<MESSAGE> ( log() << "Sparsifing RAW data on event " << _iEvt );

  try {
    
    LCCollectionVec * inputCollectionVec    = dynamic_cast < LCCollectionVec * > (evt->getCollection(_rawDataCollectionName));
    LCCollectionVec * pedestalCollectionVec = dynamic_cast < LCCollectionVec * > (evt->getCollection(_pedestalCollectionName));
    LCCollectionVec * noiseCollectionVec    = dynamic_cast < LCCollectionVec * > (evt->getCollection(_noiseCollectionName));
    LCCollectionVec * statusCollectionVec   = dynamic_cast < LCCollectionVec * > (evt->getCollection(_statusCollectionName));
    CellIDDecoder<TrackerRawDataImpl> cellDecoder(inputCollectionVec);
    

    if (isFirstEvent()) {
      
      // this is the right place to cross check wheter the pedestal and
      // the input data are at least compatible. I mean the same number
      // of detectors and the same number of pixels in each place.
      
      if ( (inputCollectionVec->getNumberOfElements() != pedestalCollectionVec->getNumberOfElements()) ||
	   (inputCollectionVec->getNumberOfElements() != _noOfDetector) ) {
	stringstream ss;
	ss << "Input data and pedestal are incompatible" << endl
	   << "Input collection has    " << inputCollectionVec->getNumberOfElements()    << " detectors," << endl
	   << "Pedestal collection has " << pedestalCollectionVec->getNumberOfElements() << " detectors," << endl
	   << "The expected number is  " << _noOfDetector << endl;
	throw IncompatibleDataSetException(ss.str());
      }


      for (int iDetector = 0; iDetector < _noOfDetector; iDetector++) {

	TrackerRawDataImpl * rawData  = dynamic_cast < TrackerRawDataImpl * >(inputCollectionVec->getElementAt(iDetector));
	TrackerDataImpl    * pedestal = dynamic_cast < TrackerDataImpl * >   (pedestalCollectionVec->getElementAt(iDetector));

	if (rawData->getADCValues().size() != pedestal->getChargeValues().size()){
	  stringstream ss;
	  ss << "Input data and pedestal are incompatible" << endl
	     << "Detector " << iDetector << " has " <<  rawData->getADCValues().size() << " pixels in the input data " << endl
	     << "while " << pedestal->getChargeValues().size() << " in the pedestal data " << endl;
	  throw IncompatibleDataSetException(ss.str());
	}

	// before continuing it is a better idea to verify that the
	// rawData is really a full frame data set and not already
	// sparsified! To do that we try to get from the cellDecoder
	// some information stored only in full frame mode, like the
	// "xMin". If xMin it is not there, then an un-caught
	// exception will be thrown and the execution stopped here.
	cellDecoder(rawData)["xMin"];
      }
      _isFirstEvent = false;
    }
    
    LCCollectionVec * sparsifiedDataCollection = new LCCollectionVec(LCIO::TRACKERDATA);
    
    for (int iDetector = 0; iDetector < _noOfDetector; iDetector++) {
      
      TrackerRawDataImpl  * rawData   = dynamic_cast < TrackerRawDataImpl * >(inputCollectionVec->getElementAt(iDetector));
      TrackerDataImpl     * pedestal  = dynamic_cast < TrackerDataImpl * >   (pedestalCollectionVec->getElementAt(iDetector));
      TrackerDataImpl     * noise     = dynamic_cast < TrackerDataImpl * >   (noiseCollectionVec->getElementAt(iDetector));
      TrackerRawDataImpl  * status    = dynamic_cast < TrackerRawDataImpl * >(statusCollectionVec->getElementAt(iDetector));
      
      TrackerDataImpl     * sparsified = new TrackerDataImpl;
      CellIDEncoder<TrackerDataImpl> sparseDataEncoder(EUTELESCOPE::ZSDATADEFAULTENCODING, sparsifiedDataCollection);
      int sensorID = static_cast<int> ( cellDecoder(rawData)["sensorID"] );
      sparseDataEncoder["sensorID"]        = sensorID;
      sparseDataEncoder["sparsePixelType"] = static_cast<int> ( _pixelType );
      sparseDataEncoder.setCellID(sparsified);

      EUTelMatrixDecoder matrixDecoder(cellDecoder, rawData);

      ShortVec::const_iterator rawIter     = rawData->getADCValues().begin();
      FloatVec::const_iterator pedIter     = pedestal->getChargeValues().begin();
      FloatVec::const_iterator noiseIter   = noise->getChargeValues().begin();
      ShortVec::const_iterator statusIter  = status->getADCValues().begin();
      int iPixel = 0;
      float sigmaCut = _sigmaCutVec[sensorID];

      if ( _pixelType == kEUTelSimpleSparsePixel ) {

	EUTelSparseDataImpl<EUTelSimpleSparsePixel>  sparseData( sparsified ) ;
	while ( rawIter != rawData->getADCValues().end() ) {
	  if (  (*statusIter) == EUTELESCOPE::GOODPIXEL ) { 
	    float data      = (*rawIter) - (*pedIter);
	    float threshold = sigmaCut * (*noiseIter);
	    if ( data > threshold  ) {
	      auto_ptr<EUTelSimpleSparsePixel> sparsePixel( new EUTelSimpleSparsePixel );
	      sparsePixel->setXCoord( matrixDecoder.getXFromIndex(iPixel) );
	      sparsePixel->setYCoord( matrixDecoder.getYFromIndex(iPixel) );
	      sparsePixel->setSignal( static_cast<short> ( data ) );
	      message<DEBUG> ( log() << (*sparsePixel.get())  ) ;
	      sparseData.addSparsePixel( sparsePixel.get() );
	    }
	  }
	  ++rawIter; 
	  ++pedIter;
	  ++noiseIter;
	  ++statusIter;
	  ++iPixel;
	}

      } else if ( _pixelType == kUnknownPixelType ) {
	throw UnknownDataTypeException("Unknown or not valid sparse pixel type");
      }

      sparsifiedDataCollection->push_back( sparsified );
    }
    evt->addCollection(sparsifiedDataCollection, _sparsifiedDataCollectionName);
  
  
    ++_iEvt;
  } catch (DataNotAvailableException& e) {
    message<ERROR> ( log() << e.what() << "\n"
		     << "Skipping this event " );
    throw SkipEventException(this);
  }
  
}
  


void EUTelRawDataSparsifier::check (LCEvent * evt) {
  // nothing to check here - could be used to fill check plots in reconstruction processor
}


void EUTelRawDataSparsifier::end() {
  message<MESSAGE> ( "Successfully finished" );

}

