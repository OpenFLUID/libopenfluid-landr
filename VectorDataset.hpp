/*
 This file is part of OpenFLUID software
 Copyright (c) 2007-2010 INRA-Montpellier SupAgro


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
 along with OpenFLUID.  If not, see <http://www.gnu.org/licenses/>.

 In addition, as a special exception, INRA gives You the additional right
 to dynamically link the code of OpenFLUID with code not covered
 under the GNU General Public License ("Non-GPL Code") and to distribute
 linked combinations including the two, subject to the limitations in this
 paragraph. Non-GPL Code permitted under this exception must only link to
 the code of OpenFLUID dynamically through the OpenFLUID libraries
 interfaces, and only for building OpenFLUID plugins. The files of
 Non-GPL Code may be link to the OpenFLUID libraries without causing the
 resulting work to be covered by the GNU General Public License. You must
 obey the GNU General Public License in all respects for all of the
 OpenFLUID code and other code used in conjunction with OpenFLUID
 except the Non-GPL Code covered by this exception. If you modify
 this OpenFLUID, you may extend this exception to your version of the file,
 but you are not obligated to do so. If you do not wish to provide this
 exception without modification, you must delete this exception statement
 from your version and license this OpenFLUID solely under the GPL without
 exception.


 == Other Usage ==

 Other Usage means a use of OpenFLUID that is inconsistent with the GPL
 license, and requires a written agreement between You and INRA.
 Licensees for Other Usage of OpenFLUID may use this file in accordance
 with the terms contained in the written agreement between You and INRA.
 */

/**
 \file VectorDataset.hpp
 \brief Header of ...

 \author Aline LIBRES <aline.libres@gmail.com>
 */

#ifndef VECTORDATASET_HPP_
#define VECTORDATASET_HPP_

#include <string>
#include <map>
#include <list>
#include <ogrsf_frmts.h>

namespace geos {
namespace geom {
class Geometry;
}
}

namespace openfluid {

namespace core {
class GeoVectorValue;
}

namespace landr {

class VectorDataset
{
  public:

    typedef std::list<std::pair<OGRFeature*, geos::geom::Geometry*> > FeaturesList_t;

  private:

    OGRDataSource* mp_DataSource;

    /**
     * List of all features of layers of this GeoVectorValue, indexed by layer index
     */
    std::map<unsigned int, FeaturesList_t> m_Features;

    /**
     * Geometries representing a collection of all the geometries of the layers of this GeoVectorValue, indexed by layer index
     */
    std::map<unsigned int, geos::geom::Geometry*> m_Geometries;

    std::string getInitializedTmpPath();

    bool isAlreadyExisting(std::string Path);

    void parse(unsigned int LayerIndex);

  public:

    /**
     * Create a new empty OGRDatasource in the openfluid temp directory
     * @param FileName The name of the file to create
     * @param DriverName The name of the OGR driver to use, default is "ESRI Shapefile"
     */
    VectorDataset(std::string FileName, std::string DriverName =
        "ESRI Shapefile");

    /**
     * Create in the openfluid temp directory a copy of Value OGRDatasource,
     * using Value filename as default filename, unless NewFileName is provided
     * @param Value The GeoVectorValue to copy
     * @param NewFileName The alternate name to use to create the file, optionnal
     */
    VectorDataset(openfluid::core::GeoVectorValue& Value,
                  std::string NewFileName = "");

    /**
     * Delete the OGRDatasource and relative files in openfluid temp directory
     */
    ~VectorDataset();

    OGRDataSource* getDataSource();

    /**
     * Write to disk a copy of the OGRDataSource
     * @param FilePath The path to the directory where writing, will be created if needed
     * @param FileName The name of the file to write
     * @param ReplaceIfExists If true and the file FilePath/FileName already exists, overwrite it
     */
    void copyToDisk(std::string FilePath, std::string FileName,
                    bool ReplaceIfExists);

    /**
     * @brief Add to DataSource an empty new layer
     *
     * @param LayerName The name of the layer to create
     * @param LayerType The type of the layer to create, default wkbUnknown
     * @param SpatialRef The coordinate system to use for the new layer, or NULL (default) if no coordinate system is available
     * @throw openfluid::base::OFException if the creation of layer failed
     */
    void addALayer(std::string LayerName = "", OGRwkbGeometryType LayerType =
        wkbUnknown,
                   OGRSpatialReference* SpatialRef = NULL);

    /**
     * @brief Get a layer of the shape.
     *
     * @param LayerIndex The index of the asked layer, default 0
     * @return The layer indexed LayerIndex
     */
    OGRLayer* getLayer(unsigned int LayerIndex = 0);

    /**
     * @brief Get the Feature definition of a layer.
     *
     * @param LayerIndex The index of the asked layer definition, default 0
     * @return The OGR Feature definition of the LayerIndex layer
     */
    OGRFeatureDefn* getLayerDef(unsigned int LayerIndex = 0);

    /**
     * @brief Add a field to a layer.
     *
     * @param FieldName The name of the field to add.
     * @param FieldType The type of the field to add (default OFTString).
     * @param LayerIndex The index of the layer to add the field, default 0
     * @throw openfluid::base::OFException if creating field failed
     */
    void addAField(std::string FieldName, OGRFieldType FieldType = OFTString,
                   unsigned int LayerIndex = 0);

    /**
     * @param LayerIndex The index of the layer to compare the type, default 0
     * @return True if the type of the layer LayerIndex is wkbLineString, false otherwise
     */
    bool isLineType(unsigned int LayerIndex = 0);

    /**
     * @param LayerIndex The index of the layer to compare the type, default 0
     * @return True if the type of the layer LayerIndex is wkbPolygon, false otherwise
     */
    bool isPolygonType(unsigned int LayerIndex = 0);

    /**
     * @brief Returns if a field exists in the LayerIndex layer.
     *
     * @param FieldName The name of the field to query
     * @param LayerIndex The index of the layer to query, default 0
     * @return True if the field FieldName exists, False otherwise
     */
    bool containsField(std::string FieldName, unsigned int LayerIndex = 0);

    /**
     * @brief Get the index of a field in the LayerIndex layer
     *
     * @param LayerIndex The index of the layer to query, default 0
     * @param FieldName The name of the field to query
     * @return The index of FieldName or -1 if field FieldName doesn't exist
     */
    int getFieldIndex(std::string FieldName, unsigned int LayerIndex = 0);

    /**
     * @brief Returns if a field is of the type FieldType in the LayerIndex layer
     *
     * @param FieldName The name of the field to query
     * @param FieldType The type of the field to query
     * @param LayerIndex The index of the layer to query, default 0
     * @return True if the field FieldName is type FieldType
     * @throw openfluid::base::OFException if the field doesn't exist.
     */
    bool isFieldOfType(std::string FieldName, OGRFieldType FieldType,
                       unsigned int LayerIndex = 0);

    /**
     * @brief Returns if a field has the value Value in the LayerIndex layer
     *
     * @param FieldName The name of the field to query
     * @param Value The value to query
     * @param LayerIndex The index of the layer to query, default 0
     * @return True if the field has at least a feature containing the value Value, False otherwise.
     */
    bool isIntValueSet(std::string FieldName, int Value,
                       unsigned int LayerIndex = 0);

    /**
     * @brief Get the list of all features of a layer of this GeoVectorValue
     *
     * @param LayerIndex The index of the layer to query, default 0
     */
    FeaturesList_t getFeatures(unsigned int LayerIndex = 0);

    /**
     * Get a Geometry representing a collection of all the geometries of the layer LayerIndex of this GeoVectorValue
     *
     * @param LayerIndex The index of the layer to query, default 0
     */
    geos::geom::Geometry* getGeometries(unsigned int LayerIndex = 0);

};

}
} // namespaces

#endif /* VECTORDATASET_HPP_ */
