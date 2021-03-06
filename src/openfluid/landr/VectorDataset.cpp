/*

  This file is part of OpenFLUID software
  Copyright(c) 2007, INRA - Montpellier SupAgro


 == GNU General Public License Usage ==

  OpenFLUID is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OpenFLUID is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OpenFLUID. If not, see <http://www.gnu.org/licenses/>.


 == Other Usage ==

  Other Usage means a use of OpenFLUID that is inconsistent with the GPL
  license, and requires a written agreement between You and INRA.
  Licensees for Other Usage of OpenFLUID may use this file in accordance
  with the terms contained in the written agreement between You and INRA.

*/


/**
  @file VectorDataset.cpp

  @author Aline LIBRES <aline.libres@gmail.com>
  @author Michael RABOTIN <michael.rabotin@supagro.inra.fr>
*/


#include <algorithm>
#include <utility>
#include <chrono>

#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/LineString.h>
#include <geos/geom/GeometryCollection.h>
#include <geos/operation/valid/IsValidOp.h>
#include <geos/geom/Point.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/operation/overlay/snap/GeometrySnapper.h>

#include <openfluid/landr/GEOSHelpers.hpp>
#include <openfluid/landr/VectorDataset.hpp>
#include <openfluid/base/FrameworkException.hpp>
#include <openfluid/base/Environment.hpp>
#include <openfluid/tools/Filesystem.hpp>
#include <openfluid/tools/DataHelpers.hpp>


namespace openfluid { namespace landr {


VectorDataset::VectorDataset(const std::string& FileName)
{
  std::string DefaultDriverName = "ESRI Shapefile";

#if (GDAL_VERSION_MAJOR >= 2)
  GDALAllRegister();
#else
  OGRRegisterAll();
#endif

  GDALDataset_COMPAT* poDS = GDALOpenRO_COMPAT(FileName.c_str());

  if (poDS != nullptr)
  {
#if (GDAL_VERSION_MAJOR >= 2)
    std::string DriverName = poDS->GetDriver()->GetDescription();
#else
    std::string DriverName = poDS->GetDriver()->GetName();
#endif


    if (DriverName != DefaultDriverName)
    {
      GDALClose_COMPAT(poDS);
      throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "\"" + DriverName + "\" driver not supported.");
    }
  }
  GDALClose_COMPAT(poDS);


  GDALDriver_COMPAT* Driver = nullptr;

#if (GDAL_VERSION_MAJOR >= 2)
  Driver = GetGDALDriverManager()->GetDriverByName(DefaultDriverName.c_str());
#else
  Driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(DefaultDriverName.c_str());
#endif

  if (!Driver)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "\"" + DefaultDriverName + "\" driver not available.");
  }

  std::string Path = getTimestampedPath(FileName);

  mp_DataSource = GDALCreate_COMPAT(Driver,Path.c_str());

  if (!mp_DataSource)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while creating " + Path + " : " +
                                              "Creation of data source failed.");
  }

#if (GDAL_VERSION_MAJOR >= 2)

#else
  mp_DataSource->SetDriver(Driver);
#endif

}


// =====================================================================
// =====================================================================


VectorDataset::VectorDataset(openfluid::core::GeoVectorValue& Value)
{
#if (GDAL_VERSION_MAJOR >= 2)
  GDALAllRegister();
#else
  OGRRegisterAll();
#endif

  GDALDataset_COMPAT* DS = Value.data();
  GDALDriver_COMPAT* Driver = DS->GetDriver();

#if (GDAL_VERSION_MAJOR >= 2)
  std::string DriverName = Driver->GetDescription();
#else
  std::string DriverName = Driver->GetName();
#endif

  if (DriverName!="ESRI Shapefile")
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "\"" + DriverName + "\" driver not supported.");
  }

#if (GDAL_VERSION_MAJOR >= 2)
  std::string Path = getTimestampedPath(openfluid::tools::Filesystem::basename(DS->GetDescription()));
#else
  std::string Path = getTimestampedPath(openfluid::tools::Filesystem::basename(DS->GetName()));
#endif

  mp_DataSource = GDALCopy_COMPAT(Driver,DS,Path.c_str());

  if (!mp_DataSource)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while creating " + Path + " : " +
                                              "Creation of data source failed.");
  }

#if (GDAL_VERSION_MAJOR >= 2)

#else
  mp_DataSource->SetDriver(Driver);
#endif


  // necessary to ensure headers are written out in an orderly way and all resources are recovered
  GDALClose_COMPAT(mp_DataSource);
  mp_DataSource = GDALOpenRW_COMPAT(Path.c_str());

  if (!mp_DataSource)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while opening " + Path + " : " +
                                              "Loading of data source failed.");
  }
}


// =====================================================================
// =====================================================================


VectorDataset::VectorDataset(const VectorDataset& Other)
{
#if (GDAL_VERSION_MAJOR >= 2)
  GDALAllRegister();
#else
  OGRRegisterAll();
#endif

  GDALDataset_COMPAT* DS = Other.source();
  GDALDriver_COMPAT* Driver = DS->GetDriver();

#if (GDAL_VERSION_MAJOR >= 2)
  std::string DriverName = Driver->GetDescription();
#else
  std::string DriverName = Driver->GetName();
#endif

  if (DriverName!="ESRI Shapefile")
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "\"" + DriverName + "\" driver not supported.");
  }

#if (GDAL_VERSION_MAJOR >= 2)
  std::string Path = getTimestampedPath(openfluid::tools::Filesystem::basename(DS->GetDescription()));
#else
  std::string Path = getTimestampedPath(openfluid::tools::Filesystem::basename(DS->GetName()));
#endif

  mp_DataSource = GDALCopy_COMPAT(Driver,DS,Path.c_str());

  if (!mp_DataSource)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while creating " + Path + " : " +
                                              "Creation of data source failed.");
  }

#if (GDAL_VERSION_MAJOR >= 2)

#else
  mp_DataSource->SetDriver(Driver);
#endif


  // necessary to ensure headers are written out in an orderly way and all resources are recovered
  GDALClose_COMPAT(mp_DataSource);
  mp_DataSource = GDALOpenRW_COMPAT(Path.c_str());

  if (!mp_DataSource)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while opening " + Path + " : " +
                                              "Loading of data source failed.");
  }
}


// =====================================================================
// =====================================================================


std::string VectorDataset::getTimestampedPath(const std::string& OriginalFileName)
{
  std::string FileWOExt = openfluid::tools::Filesystem::basename(OriginalFileName);
  std::string Ext = openfluid::tools::Filesystem::extension(OriginalFileName);

  std::chrono::system_clock::time_point TimePoint = std::chrono::system_clock::now();
  std::time_t Time = std::chrono::system_clock::to_time_t(TimePoint);

  char NowChar[16];
  std::strftime(NowChar, sizeof(NowChar), "%Y%m%dT%H%M%S", std::localtime(&Time));
  std::string Now(NowChar);


  return openfluid::core::GeoValue::computeAbsolutePath(
      getInitializedTmpPath(),
      FileWOExt + "_" + Now + '.'+Ext);
}


// =====================================================================
// =====================================================================


std::string VectorDataset::getInitializedTmpPath()
{
  std::string TmpPath = openfluid::base::Environment::getTempDir();

  if (!openfluid::tools::Filesystem::isDirectory(TmpPath))
  {
    openfluid::tools::Filesystem::makeDirectory(TmpPath);
  }

  return TmpPath;
}


// =====================================================================
// =====================================================================


bool VectorDataset::isAlreadyExisting(const std::string& Path)
{
  GDALDataset_COMPAT* DS = GDALOpenRO_COMPAT(Path.c_str());;

  if (DS)
  {
    GDALClose_COMPAT(DS);
    return true;
  }

  return false;
}


// =====================================================================
// =====================================================================


VectorDataset::~VectorDataset()
{
  GDALDriver_COMPAT* Driver = mp_DataSource->GetDriver();

#if (GDAL_VERSION_MAJOR >= 2)
  std::string Path = mp_DataSource->GetDescription();
#else
  std::string Path = mp_DataSource->GetName();
#endif


  // if the Datasource was not flushed to disk
  if (isAlreadyExisting(Path))
  {
    GDALClose_COMPAT(mp_DataSource);
    GDALDelete_COMPAT(Driver,Path.c_str());
  }
}


// =====================================================================
// =====================================================================


GDALDataset_COMPAT* VectorDataset::source()
{
  return mp_DataSource;
}


// =======================================y==============================
// =====================================================================


GDALDataset_COMPAT* VectorDataset::source() const
{
  return mp_DataSource;
}


// =====================================================================
// =====================================================================


void VectorDataset::copyToDisk(const std::string& FilePath,
                               const std::string& FileName,
                               bool ReplaceIfExists)
{
  GDALDriver_COMPAT* Driver = mp_DataSource->GetDriver();

  if (!openfluid::tools::Filesystem::isDirectory(FilePath))
  {
    openfluid::tools::Filesystem::makeDirectory(FilePath);
  }

  std::string Path = openfluid::core::GeoValue::computeAbsolutePath(FilePath,
                                                                    FileName);

  GDALDataset_COMPAT* DS = GDALOpenRO_COMPAT(Path.c_str());

  if (DS)
  {
    if (ReplaceIfExists)
    {
      GDALDriver_COMPAT* Driver = DS->GetDriver();
      GDALClose_COMPAT(DS);
      GDALDelete_COMPAT(Driver,Path.c_str());
    }
    else
    {
      GDALClose_COMPAT(DS);
      throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "Error while creating " + Path +
                                                " : This VectorDataset already exists.");
    }
  }

  GDALDataset_COMPAT* NewDS = nullptr;

  NewDS = GDALCopy_COMPAT(Driver,mp_DataSource,Path.c_str());

  if (!NewDS)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while creating " + Path + " : Copying of OGRDataSource failed.");
  }

  // necessary to ensure headers are written out in an orderly way and all resources are recovered
  GDALClose_COMPAT(NewDS);
}


// =====================================================================
// =====================================================================


void VectorDataset::addALayer(std::string LayerName,
                              OGRwkbGeometryType LayerType,
                              OGRSpatialReference* SpatialRef)
{
#if (GDAL_VERSION_MAJOR >= 2)
  std::string Path = mp_DataSource->GetDescription();
#else
  std::string Path = mp_DataSource->GetName();
#endif


  if (mp_DataSource->GetLayerByName(LayerName.c_str()) != nullptr)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while adding a layer to " + Path +
                                              ": a layer named " + LayerName + " already exists.");
  }

  OGRLayer* Layer = mp_DataSource->CreateLayer(LayerName.c_str(), SpatialRef,
                                               LayerType, nullptr);

  if (!Layer)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while adding a layer to " + Path +
                                              ": creation of layer " + LayerName + " failed.");
  }

  // necessary to ensure headers are written out in an orderly way and all resources are recovered
  GDALClose_COMPAT(mp_DataSource);

  mp_DataSource = GDALOpenRW_COMPAT(Path.c_str());

  if (!mp_DataSource)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Error while opening " + Path + " : Opening of OGRDataSource failed.");
  }
}


// =====================================================================
// =====================================================================


OGRLayer* VectorDataset::layer(unsigned int LayerIndex)
{
  return mp_DataSource->GetLayer(LayerIndex);
}


// =====================================================================
// =====================================================================


OGRFeatureDefn* VectorDataset::layerDef(unsigned int LayerIndex)
{
  return layer(LayerIndex)->GetLayerDefn();
}


// =====================================================================
// =====================================================================


void VectorDataset::addAField(const std::string& FieldName,
                              OGRFieldType FieldType,
                              unsigned int LayerIndex)
{
  OGRFieldDefn Field(FieldName.c_str(), FieldType);

  if (layer(LayerIndex)->CreateField(&Field) != OGRERR_NONE)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Creating field \"" + FieldName + "\" failed.");
  }
}


// =====================================================================
// =====================================================================


bool VectorDataset::isLineType(unsigned int LayerIndex)
{
  return layerDef(LayerIndex)->GetGeomType() == wkbLineString;
}


// =====================================================================
// =====================================================================


bool VectorDataset::isPolygonType(unsigned int LayerIndex)
{
  return layerDef(LayerIndex)->GetGeomType() == wkbPolygon;
}


// =====================================================================
// =====================================================================


bool VectorDataset::containsField(const std::string& FieldName,
                                  unsigned int LayerIndex)
{
  return layerDef(LayerIndex)->GetFieldIndex(FieldName.c_str()) != -1;
}


// =====================================================================
// =====================================================================


int VectorDataset::getFieldIndex(const std::string& FieldName,
                                 unsigned int LayerIndex)
{
  return layerDef(LayerIndex)->GetFieldIndex(FieldName.c_str());
}


// =====================================================================
// =====================================================================


bool VectorDataset::isFieldOfType(const std::string& FieldName,
                                  OGRFieldType FieldType,
                                  unsigned int LayerIndex)
{
  if (!containsField(FieldName))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Field \"" + FieldName + "\" is not set.");
  }

  return layerDef(LayerIndex)->GetFieldDefn(getFieldIndex(FieldName))->GetType() == FieldType;
}


// =====================================================================
// =====================================================================


bool VectorDataset::isIntValueSet(const std::string& FieldName,
                                  int Value,
                                  unsigned int LayerIndex)
{
  if (!isFieldOfType(FieldName, OFTInteger, LayerIndex) &&
      !isFieldOfType(FieldName, GDALOFTInteger64_COMPAT, LayerIndex))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Field \"" + FieldName + "\" is not set or is not of type Int.");
  }

  int CatIndex = layerDef(LayerIndex)->GetFieldIndex(FieldName.c_str());

  OGRLayer* Layer = layer(LayerIndex);

  Layer->ResetReading();

  OGRFeature* Feat;
  while ((Feat = Layer->GetNextFeature()) != nullptr)
  {
    if (Feat->GetFieldAsInteger(CatIndex) == Value)
    {
      OGRFeature::DestroyFeature(Feat);
      return true;
    }
    OGRFeature::DestroyFeature(Feat);
  }

  return false;
}


// =====================================================================
// =====================================================================


VectorDataset::FeaturesList_t VectorDataset::features(unsigned int LayerIndex)
{
  if (!m_Features.count(LayerIndex))
  {
    parse(LayerIndex);
  }

  return m_Features.at(LayerIndex);
}


// =====================================================================
// =====================================================================


geos::geom::Geometry* VectorDataset::geometries(unsigned int LayerIndex)
{
  if (!m_Geometries.count(LayerIndex))
  {
    parse(LayerIndex);
  }

  return m_Geometries.at(LayerIndex);
}


// =====================================================================
// =====================================================================


// TODO add an option to allow choice of checking validity or not (because it's time consuming)
void VectorDataset::parse(unsigned int LayerIndex)
{
  std::vector<geos::geom::Geometry*>* Geoms = new std::vector<geos::geom::Geometry*>();

  // TODO Should this line be moved?
  setlocale(LC_NUMERIC, "C");

  OGRLayer* Layer = layer(LayerIndex);

  Layer->ResetReading();

  OGRFeature* Feat;

  FeaturesList_t List;
  m_Features.insert(std::make_pair(LayerIndex, List));

  // GetNextFeature returns a copy of the feature
  while ((Feat = Layer->GetNextFeature()) != nullptr)
  {
    OGRGeometry* OGRGeom = Feat->GetGeometryRef();

    if (OGRGeom->getGeometryType()==wkbPolygon && OGRGeom->getGeometryType()!=wkbMultiPolygon)
    {
      OGRPolygon *Polygon = (OGRPolygon *) OGRGeom;

      if (Polygon->getExteriorRing()->getNumPoints() < 4)
      {
        throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                  "Unable to build the polygon with FID " +
                                                  openfluid::tools::convertValue(Feat->GetFID()));
      }

    }

    // c++ cast doesn't work (have to use C-style casting instead)
    geos::geom::Geometry* GeosGeom = (geos::geom::Geometry*)openfluid::landr::convertOGRGeometryToGEOS(OGRGeom);

    geos::operation::valid::IsValidOp ValidOp(GeosGeom);

    if (!ValidOp.isValid())
    {
      throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                ValidOp.getValidationError()->toString() + " \nwhile parsing " +
                                                GeosGeom->toString());
    }

    geos::geom::Geometry* GeomClone = GeosGeom->clone().release();
    OGRFeature* FeatClone = Feat->Clone();

    Geoms->push_back(GeomClone);
    m_Features.at(LayerIndex).push_back(std::make_pair(FeatClone,GeomClone));

    // destroying the feature destroys also the associated OGRGeom
    OGRFeature::DestroyFeature(Feat);
    delete GeosGeom;
  }

  // ! do not use buildGeometry, because it may build a MultiPolygon if all geometries
  //are Polygons, what may produce an invalid MultiPolygon!
  // (because the boundaries of any two Polygons of a valid MultiPolygon may touch,
  //*but only at a finite number of points*)
  geos::geom::GeometryCollection* GTMP = geos::geom::GeometryFactory::getDefaultInstance()->createGeometryCollection(Geoms);
  m_Geometries.insert(std::make_pair(LayerIndex,GTMP));

  geos::operation::valid::IsValidOp ValidOpColl(m_Geometries.at(LayerIndex));

  if (!ValidOpColl.isValid())
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              ValidOpColl.getValidationError()->toString() + " \nwhile creating " +
                                              m_Geometries.at(LayerIndex)->toString());
  }
}


// =====================================================================
// =====================================================================


bool VectorDataset::isPointType(unsigned int LayerIndex)
{
  return layerDef(LayerIndex)->GetGeomType() == wkbPoint;
}


// =====================================================================
// =====================================================================


void VectorDataset::setIndexIntField(const std::string& FieldName,
                                     int BeginValue,
                                     unsigned int LayerIndex)

{
  if (!isFieldOfType(FieldName, OFTInteger, LayerIndex)  &&
      !isFieldOfType(FieldName, GDALOFTInteger64_COMPAT, LayerIndex))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "Field \"" + FieldName + "\" is not set or is not of type Int.");
  }

  OGRLayer* Layer = layer(LayerIndex);

  if (! Layer->TestCapability( "RandomWrite"))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "Unable to update the Field \"" + FieldName +"\"");
  }

  Layer->ResetReading();

  OGRFeature* Feat;
  while ((Feat = Layer->GetNextFeature()) != nullptr)
  {
    Feat->SetField(FieldName.c_str(),BeginValue);
    Layer->SetFeature(Feat);
    OGRFeature::DestroyFeature(Feat);
    BeginValue++;
  }

}


// =====================================================================
// =====================================================================


OGREnvelope VectorDataset::envelope()
{
  OGREnvelope Envelope;
  layer()->GetExtent(&Envelope);

  return Envelope;

}


// =====================================================================
// =====================================================================


void VectorDataset::snapVertices(double Threshold,unsigned int LayerIndex)
{
  if (isLineType())
  {
    snapLineNodes(Threshold,LayerIndex);
  }
  else if (isPolygonType())
  {
    snapPolygonVertices(Threshold,LayerIndex);
  }
  else
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "this VectorDataset is nor Line nor Polygon Type");
  }
}


// =====================================================================
// =====================================================================


void VectorDataset::snapLineNodes(double Threshold,unsigned int LayerIndex)
{

  FeaturesList_t Features = features(LayerIndex);

  for (FeaturesList_t::iterator it = Features.begin(); it != Features.end(); ++it)
  {
    geos::geom::Geometry *Geom=geometries();

    geos::geom::LineString *CurrentLine=(dynamic_cast<geos::geom::LineString*>((*it).second));
    // get a vector of StartPoint and EndPoint of all Geometries
    // except the current Geometry
    std::vector<geos::geom::Point*> vPoints;
    unsigned int iEnd = Geom->getNumGeometries();

    for (unsigned int i = 0; i < iEnd; i++)
    {
      if (!(const_cast<geos::geom::Geometry*>(Geom->getGeometryN(i)))->equals((*it).second))
      {
        geos::geom::LineString *Line =
            dynamic_cast<geos::geom::LineString*>(const_cast<geos::geom::Geometry*>(Geom->getGeometryN(i)));
        vPoints.push_back(Line->getStartPoint().release());
        vPoints.push_back(Line->getEndPoint().release());
      }
    }

    geos::geom::Point* PStart = CurrentLine->getStartPoint().release();
    geos::geom::Point* PEnd = CurrentLine->getEndPoint().release();
    geos::geom::Point* NewPStart = nullptr;
    geos::geom::Point* NewPEnd = nullptr;

    for (unsigned int j = 0; j < vPoints.size(); j++)
    {
      if (!PStart->equals(vPoints.at(j)) && PStart->equalsExact(vPoints.at(j),Threshold))
      {
        NewPStart = vPoints.at(j);
      }
      if (!PEnd->equals(vPoints.at(j)) && PEnd->equalsExact(vPoints.at(j),Threshold))
      {
        NewPEnd = vPoints.at(j);
      }
    }

    if (!NewPStart && !NewPEnd)
    {
      continue;  // TODO to be replaced!
    }

    geos::geom::CoordinateSequence* CoordSeq=CurrentLine->getCoordinates().release();
    if (NewPStart)
    {
      CoordSeq->setAt(*NewPStart->getCoordinate(),0);
    }

    if (NewPEnd)
    {
      CoordSeq->setAt(*NewPEnd->getCoordinate(),CoordSeq->getSize()-1);
    }

    geos::geom::LineString* NewLine = geos::geom::GeometryFactory::getDefaultInstance()->createLineString(CoordSeq);
    OGRGeometry* OGRGeom =
      openfluid::landr::convertGEOSGeometryToOGR((GEOSGeom) dynamic_cast<geos::geom::Geometry*>(NewLine));
    OGRFeature* OGRFeat = (*it).first;
    OGRFeat->SetGeometry(OGRGeom);
    geos::geom::Geometry* NewLineClone = NewLine->clone().release();
    OGRFeature* OGRFeatClone = OGRFeat->Clone();
    (*it) = std::make_pair<OGRFeature*, geos::geom::Geometry*>
    (dynamic_cast<OGRFeature *>(OGRFeat), dynamic_cast<geos::geom::Geometry*>(NewLineClone));
    mp_DataSource->GetLayer(LayerIndex)->SetFeature(OGRFeatClone);
    m_Geometries.clear();

    delete NewLine;
    OGRFeature::DestroyFeature(OGRFeat);
    delete PStart;
    delete PEnd;
    delete NewPStart;
    delete NewPEnd;
    delete CurrentLine;

    try
    {
      parse(LayerIndex);
    }
    catch (std::exception& e)
    {
      throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "Unable to parse the VectorDataset (" + std::string(e.what()) + ")");
    }
  }
}


// =====================================================================
// =====================================================================


void VectorDataset::snapPolygonVertices(double Threshold,unsigned int LayerIndex)
{
  FeaturesList_t Features = features(LayerIndex);

  for (FeaturesList_t::iterator it = Features.begin(); it != Features.end(); ++it)
  {
    // FIXME Crashes may happen here
    geos::geom::Geometry* Geom = geometries();
    geos::geom::Polygon* CurrentPolygon = (dynamic_cast<geos::geom::Polygon*>((*it).second));

    // get a vector of the Coordinates of the vertices of all Geometries
    // except the current Geometry
    std::vector<geos::geom::Coordinate> vCoor;
    unsigned int iEnd = Geom->getNumGeometries();

    for (unsigned int i = 0; i < iEnd; i++)
    {

      if (!(const_cast<geos::geom::Geometry*>(Geom->getGeometryN(i)))->equals((*it).second))
      {
        geos::geom::Polygon *Polygon=
            (dynamic_cast<geos::geom::Polygon*>(const_cast<geos::geom::Geometry*>(Geom->getGeometryN(i))));

        if (Polygon)
        {
          geos::geom::CoordinateArraySequence(*(Polygon->getCoordinates().get())).toVector(vCoor);
        }
        delete Polygon;
      }
    }

    std::vector<geos::geom::Coordinate> vCoorCurrentGeom;
    geos::geom::CoordinateArraySequence(*(CurrentPolygon->getCoordinates().get())).toVector(vCoorCurrentGeom);

    geos::geom::CoordinateSequence* CoordSeq = CurrentPolygon->getCoordinates().release();
    for (unsigned int j = 0; j < vCoorCurrentGeom.size(); j++)
    {
      for (unsigned int h = 0; h < vCoor.size(); h++)
      {
        if (!vCoorCurrentGeom.at(j).equals(vCoor.at(h)) &&
            vCoorCurrentGeom.at(j).distance(vCoor.at(h)) > 0 &&
            vCoorCurrentGeom.at(j).distance(vCoor.at(h)) < Threshold)
        {
          CoordSeq->setAt(vCoor.at(h),j);
        }
      }
    }

    geos::geom::Polygon* NewPolygon=geos::geom::GeometryFactory::getDefaultInstance()->createPolygon(
        geos::geom::GeometryFactory::getDefaultInstance()->createLinearRing(CoordSeq),nullptr);

    OGRGeometry* OGRGeom =
        openfluid::landr::convertGEOSGeometryToOGR((GEOSGeom) dynamic_cast<geos::geom::Geometry*>(NewPolygon));
    OGRFeature* OGRFeat = (*it).first;

    OGRFeat->SetGeometry(OGRGeom);
    OGRFeature* OGRFeatClone = OGRFeat->Clone();
    geos::geom::Geometry* NewPolygonClone = NewPolygon->clone().release();
    (*it) = std::make_pair<OGRFeature*, geos::geom::Geometry*>
    (dynamic_cast<OGRFeature *>(OGRFeatClone), dynamic_cast<geos::geom::Geometry*>(NewPolygonClone));

    mp_DataSource->GetLayer(LayerIndex)->SetFeature(OGRFeatClone);

    OGRFeature::DestroyFeature(OGRFeatClone);
    delete NewPolygonClone;
    delete CoordSeq;
    delete CurrentPolygon;

    m_Geometries.clear();
    try
    {
      parse(LayerIndex);

    }
    catch (std::exception& e)
    {
      throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "Unable to parse the VectorDataset (" + std::string(e.what()) + ")");
    }
  }
}


// =====================================================================
// =====================================================================


std::string VectorDataset::checkTopology(double Threshold, unsigned int LayerIndex)
{
  if ( ! isPolygonType(LayerIndex))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"the VectorDataset is not Polygon type");
  }

  std::string ErrorMsg;

  // TODO Should this line be moved?
  setlocale(LC_NUMERIC, "C");

  OGRLayer* Layer = layer(LayerIndex);

  Layer->ResetReading();

  OGRFeature* Feat;

  FeaturesList_t List;
  m_Features.insert(std::make_pair(LayerIndex, List));

  // GetNextFeature returns a copy of the feature
  while ((Feat = Layer->GetNextFeature()) != nullptr)
  {
    OGRGeometry* OGRGeom = Feat->GetGeometryRef();

    // c++ cast doesn't work (have to use C-style casting instead)
    geos::geom::Geometry* GeosGeom = (geos::geom::Geometry*) openfluid::landr::convertOGRGeometryToGEOS(OGRGeom);

    geos::operation::valid::IsValidOp ValidOp(GeosGeom);

    if (!ValidOp.isValid())
    {
      ErrorMsg += "\n " + ValidOp.getValidationError()->toString() +
                  " FID "+openfluid::tools::convertValue(Feat->GetFID());
    }

    // destroying the feature destroys also the associated OGRGeom
    OGRFeature::DestroyFeature(Feat);
    delete GeosGeom;
  }

  // test if overlap
  std::list<std::pair<OGRFeature*,OGRFeature*>> lOverlaps=findOverlap(LayerIndex);
  std::list<std::pair<OGRFeature*,OGRFeature*>>::iterator it= lOverlaps.begin();
  std::list<std::pair<OGRFeature*,OGRFeature*>>::iterator ite= lOverlaps.end();

  for(;it != ite; ++it)
  {
    ErrorMsg += "\nPolygon FID " + openfluid::tools::convertValue((*it).first->GetFID()) +
                " overlaps with Polygon FID " + openfluid::tools::convertValue((*it).second->GetFID());
  }

  // test if gap
  std::list<std::pair<OGRFeature*,OGRFeature*>> lGaps = findGap(Threshold,LayerIndex);
  std::list<std::pair<OGRFeature*,OGRFeature*>>::iterator jt = lGaps.begin();
  std::list<std::pair<OGRFeature*,OGRFeature*>>::iterator jte = lGaps.end();

  for(;jt != jte; ++jt)
  {
    ErrorMsg += "\nPolygon FID " + openfluid::tools::convertValue((*jt).first->GetFID()) +
                " has a gap with Polygon FID "+ openfluid::tools::convertValue((*jt).second->GetFID());
  }

  return ErrorMsg;
}


// =====================================================================
// =====================================================================


std::list<std::pair<OGRFeature*, OGRFeature*> > VectorDataset::findOverlap(unsigned int LayerIndex)
{
  if (!isPolygonType(LayerIndex))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"the VectorDataset is not Polygon type.");
  }

  m_Features.clear();
  m_Geometries.clear();
  std::list<std::pair<OGRFeature*,OGRFeature*>> lOverlaps;
  FeaturesList_t Features = features(LayerIndex);

  for (FeaturesList_t::iterator it = Features.begin(); it != Features.end(); ++it)
  {
    FeaturesList_t FeaturesToTest=features(LayerIndex);
    for (FeaturesList_t::iterator jt = FeaturesToTest.begin(); jt != FeaturesToTest.end(); ++jt)
    {
      //test if overlap
      if (!(*it).first->GetGeometryRef()->Equals((*jt).first->GetGeometryRef())
          && (*it).first->GetGeometryRef()->Overlaps((*jt).first->GetGeometryRef()))
      {
        std::list<std::pair<OGRFeature*, OGRFeature*>>::iterator lt;
        std::list<std::pair<OGRFeature*, OGRFeature*>>::iterator lt2;
        std::pair<OGRFeature*, OGRFeature*> pair1 ((*it).first, (*jt).first);
        std::pair<OGRFeature*, OGRFeature*> pair2 ((*jt).first, (*it).first);

        lt = std::find(lOverlaps.begin(), lOverlaps.end(), pair1);
        lt2 = std::find (lOverlaps.begin(), lOverlaps.end(), pair2);
        if (lt == lOverlaps.end() && lt2 == lOverlaps.end())
        {
          lOverlaps.push_back(pair1);
        }

      }
    }
  }

  return lOverlaps;
}


// =====================================================================
// =====================================================================


std::list<std::pair<OGRFeature*,OGRFeature*> > VectorDataset::findGap(double Threshold, unsigned int LayerIndex)
{
  if ( ! isPolygonType(LayerIndex))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"the VectorDataset is not Polygon type.");
  }

  m_Features.clear();
  m_Geometries.clear();
  std::list<std::pair<OGRFeature*,OGRFeature*> > lGaps;
  FeaturesList_t Features = features(LayerIndex);

  for (FeaturesList_t::iterator it = Features.begin(); it != Features.end(); ++it)
  {
    FeaturesList_t FeaturesToTest = features(LayerIndex);
    for (FeaturesList_t::iterator jt = FeaturesToTest.begin(); jt != FeaturesToTest.end(); ++jt)
    {
      //test if Gap (under the threshold)
      if (!(*it).first->GetGeometryRef()->Equals((*jt).first->GetGeometryRef()) &&
          !(*it).first->GetGeometryRef()->Touches((*jt).first->GetGeometryRef()) &&
          !(*it).first->GetGeometryRef()->Overlaps((*jt).first->GetGeometryRef()) &&
          (*it).first->GetGeometryRef()->Distance((*jt).first->GetGeometryRef()) < Threshold)
      {
        std::list<std::pair<OGRFeature*,OGRFeature*>>::iterator lt;
        std::list<std::pair<OGRFeature*,OGRFeature*>>::iterator lt2;
        std::pair<OGRFeature*,OGRFeature*> pair1((*it).first, (*jt).first);
        std::pair<OGRFeature*,OGRFeature*> pair2((*jt).first, (*it).first);

        lt = std::find(lGaps.begin(), lGaps.end(), pair1);
        lt2 = std::find (lGaps.begin(), lGaps.end(), pair2);
        if (lt == lGaps.end() && lt2 == lGaps.end())
        {
          lGaps.push_back(pair1);
        }
      }
    }
  }

  return lGaps;
}


// =====================================================================
// =====================================================================


void VectorDataset::cleanOverlap(double Threshold, unsigned int LayerIndex)
{
  if ( ! isPolygonType(LayerIndex))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"the VectorDataset is not Polygon type.");
  }

  m_Features.clear();
  m_Geometries.clear();
  std::list<std::pair<OGRFeature*,OGRFeature*> > lOverlaps = findOverlap();

  int lSize=lOverlaps.size();
  for (int i=0; i<lSize;i++)
  {
    // find the OGRFeature and the OGRGeometry in m_Features
    OGRFeature *Feat1=lOverlaps.front().first;
    OGRFeature *Feat2=lOverlaps.front().second;

    geos::geom::Geometry* Geom1 =
        (geos::geom::Geometry*) openfluid::landr::convertOGRGeometryToGEOS(Feat1->GetGeometryRef());
    geos::geom::Geometry* Geom2 =
        (geos::geom::Geometry*) openfluid::landr::convertOGRGeometryToGEOS(Feat2->GetGeometryRef());

    geos::geom::Geometry * Diff= Geom1->difference(Geom2).release();

    // snap Diff with Geom2
    geos::operation::overlay::snap::GeometrySnapper geomSnapper(*const_cast<geos::geom::Geometry*>(Geom2));
    std::unique_ptr<geos::geom::Geometry> snapEntityGeom2=geomSnapper.snapTo(*Diff,Threshold);

    geos::geom::Geometry* snappedEntityGeom2=snapEntityGeom2.release();

    OGRGeometry* OGRGeom1 = openfluid::landr::convertGEOSGeometryToOGR((GEOSGeom) Diff);
    OGRGeometry* OGRGeom2 = openfluid::landr::convertGEOSGeometryToOGR((GEOSGeom) snappedEntityGeom2);


    Feat1->SetGeometry(OGRGeom1);
    Feat2->SetGeometry(OGRGeom2);

    OGRFeature* Feat1Clone = Feat1->Clone();
    OGRFeature* Feat2Clone = Feat2->Clone();

    mp_DataSource->GetLayer(LayerIndex)->SetFeature(Feat1Clone);
    mp_DataSource->GetLayer(LayerIndex)->SetFeature(Feat2Clone);

    OGRFeature::DestroyFeature(Feat1);
    OGRFeature::DestroyFeature(Feat2);

    delete snappedEntityGeom2;
    delete Diff;
    delete Geom1;
    delete Geom2;

    // clean structures and find overlap
    m_Features.clear();
    m_Geometries.clear();

    try
    {
      parse(LayerIndex);
    }
    catch (std::exception& e)
    {
      throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                                "Unable to parse the VectorDataset ("  + std::string(e.what()) + ")");
    }

    lOverlaps.clear();
    lOverlaps=findOverlap();
  }

  try
  {
    snapPolygonVertices(Threshold);
  }
  catch (std::exception& e)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "Unable to clean the VectorDataset ()" + std::string(e.what()) + ")");
  }
}


// =====================================================================
// =====================================================================


std::list<OGRFeature*> VectorDataset::hasDuplicateGeometry(unsigned int LayerIndex)
{
  std::list<OGRFeature*> lDuplicate;
  FeaturesList_t Features = features(LayerIndex);

  for (FeaturesList_t::iterator it = Features.begin(); it != Features.end(); ++it)
  {
    geos::geom::Geometry* Geom = geometries();
    unsigned int iEnd = Geom->getNumGeometries();
    int DuplicateGeom = 0;

    for (unsigned int i = 0; i < iEnd; i++)
    {
      if ((const_cast<geos::geom::Geometry*>(Geom->getGeometryN(i)))->equals((*it).second))
      {
        DuplicateGeom++;
      }
    }

    if (DuplicateGeom > 1)
    {
      lDuplicate.push_back((*it).first);
    }
  }

  return lDuplicate;
}


} }  // namespaces
