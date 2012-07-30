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
 \file LandREntity.cpp
 \brief Implements ...

 \author Aline LIBRES <aline.libres@gmail.com>
 */

#include "LandREntity.hpp"

#include <openfluid/base/OFException.hpp>

namespace openfluid {
namespace landr {

// =====================================================================
// =====================================================================

LandREntity::LandREntity() :
    mp_Feature(0), mp_SelfId(0), mp_Centroide(0)
{

}

// =====================================================================
// =====================================================================

LandREntity::LandREntity(OGRFeature* Feat) :
    mp_Feature(Feat), mp_SelfId(0), mp_Centroide(0)
{

}

// =====================================================================
// =====================================================================

LandREntity::~LandREntity()
{
  if (mp_Feature)
    OGRFeature::DestroyFeature(mp_Feature);

  delete mp_SelfId;
}

// =====================================================================
// =====================================================================

OGRFeature* LandREntity::getFeature()
{
  return mp_Feature;
}

// =====================================================================
// =====================================================================

unsigned int LandREntity::getSelfId()
{
  if (!mp_SelfId)
  {
    if (mp_Feature && mp_Feature->GetFieldIndex("SELF_ID") != -1)
      mp_SelfId = new unsigned int(mp_Feature->GetFieldAsInteger("SELF_ID"));
    else
      throw openfluid::base::OFException("OpenFLUID Framework",
                                         "LandREntity::getSelfId",
                                         "Cannot get SELF_ID field.");
  }

  return *mp_SelfId;
}

// =====================================================================
// =====================================================================

geos::geom::Point* LandREntity::getCentroide() const
{
  return mp_Centroide;
}

// =====================================================================
// =====================================================================

double LandREntity::getArea()
{
  return m_Area;
}

// =====================================================================
// =====================================================================

bool LandREntity::getAttributeValue(std::string AttributeName,
                                    boost::any& Value)
{
  if (m_Attributes.count(AttributeName))
  {
    Value = m_Attributes.find(AttributeName)->second;
    return true;
  }

  return false;
}

// =====================================================================
// =====================================================================

bool LandREntity::setAttributeValue(std::string AttributeName, boost::any Value)
{
  if (m_Attributes.count(AttributeName))
  {
    m_Attributes[AttributeName] = Value;
    return true;
  }

  return false;
}

// =====================================================================
// =====================================================================

}// namespace landr
} /* namespace openfluid */
