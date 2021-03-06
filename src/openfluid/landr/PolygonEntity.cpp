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
  @file PolygonEntity.cpp

  @author Aline LIBRES <aline.libres@gmail.com>
  @author Michael RABOTIN <michael.rabotin@supagro.inra.fr>
*/


#include <algorithm>

#include <geos/geom/Polygon.h>
#include <geos/geom/LineString.h>
#include <geos/geom/MultiLineString.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Point.h>
#include <geos/planargraph/Node.h>
#include <geos/planargraph/DirectedEdge.h>
#include <geos/geom/CoordinateArraySequence.h>

#include <openfluid/landr/PolygonEntity.hpp>
#include <openfluid/landr/PolygonEdge.hpp>
#include <openfluid/landr/LandRTools.hpp>
#include <openfluid/landr/LineStringEntity.hpp>
#include <openfluid/landr/LineStringGraph.hpp>
#include <openfluid/base/FrameworkException.hpp>


namespace openfluid { namespace landr {


PolygonEntity::PolygonEntity(const geos::geom::Geometry* NewPolygon, unsigned int OfldId) :
  LandREntity(NewPolygon, OfldId), mp_NeighboursMap(0),
  mp_LineStringNeighboursMap(0)
{
  if (mp_Geom->getGeometryTypeId() != geos::geom::GEOS_POLYGON)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"Geometry is not a Polygon");
  }

  mp_Polygon = dynamic_cast<geos::geom::Polygon*>(const_cast<geos::geom::Geometry*>(mp_Geom));

  if (!mp_Polygon->isValid())
  {
    delete mp_Centroid;

    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"Polygon is not valid");
  }
}


// =====================================================================
// =====================================================================


PolygonEntity::~PolygonEntity()
{
  delete mp_NeighboursMap;
  delete mp_LineStringNeighboursMap;
}


// =====================================================================
// =====================================================================


PolygonEntity* PolygonEntity::clone()
{
  PolygonEntity* Clone = new PolygonEntity(mp_Geom->clone().release(), m_OfldId); // why not release?

  Clone->m_PolyEdges = m_PolyEdges;

  return Clone;
}


// =====================================================================
// =====================================================================


const geos::geom::Polygon* PolygonEntity::polygon() const
{
  return mp_Polygon;
}


// =====================================================================
// =====================================================================


void PolygonEntity::addEdge(PolygonEdge& Edge)
{
  Edge.addFace(*this);
  m_PolyEdges.push_back(&Edge);

  mp_NeighboursMap = 0;

  mp_LineStringNeighboursMap = 0;
}


// =====================================================================
// =====================================================================


void PolygonEntity::removeEdge(PolygonEdge* Edge)
{
  std::vector<PolygonEdge*>::iterator itEdge = std::find(m_PolyEdges.begin(),m_PolyEdges.end(),Edge);
  if (itEdge != m_PolyEdges.end())
  {
    m_PolyEdges.erase(itEdge);
  }
  else
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"Edge doesn't exist in Edge vector.");
  }

  mp_NeighboursMap = 0;
  mp_LineStringNeighboursMap = 0;

  delete Edge;
}


// =====================================================================
// =====================================================================


std::vector<geos::geom::LineString*> PolygonEntity::computeLineIntersectionsWith(PolygonEntity& Other)
{
  std::vector<geos::geom::LineString*> Lines;

  if (mp_Polygon->relate(Other.mp_Polygon, "FFT" "F1*" "***"))
  {
    //std::unique_ptr<geos::geom::Geometry> SharedGeom = mp_Polygon->intersection(Other.mp_Polygon);
    // ^ previous solution with unique_ptr was generating a memory leak at ptr release
    geos::geom::Geometry* SharedGeom = mp_Polygon->intersection(Other.mp_Polygon).release();

    Lines = *LandRTools::computeMergedLineStringsFromGeometry(SharedGeom);

    delete SharedGeom;
  }

  return Lines;
}


// =====================================================================
// =====================================================================


PolygonEdge* PolygonEntity::findEdgeLineIntersectingWith(geos::geom::LineString& Segment)
{
  std::vector<PolygonEdge*>::iterator it = m_PolyEdges.begin();
  std::vector<PolygonEdge*>::iterator ite = m_PolyEdges.end();

  int posi=0;
  for (; it != ite; ++it)
  {
    posi++;
    geos::geom::LineString* thisLine = (*it)->line();

    if (Segment.relate(thisLine, "1**" "***" "***"))
    {
      return *it;
    }
  }

  return nullptr;
}


// =====================================================================
// =====================================================================


const PolygonEntity::NeighboursMap_t* PolygonEntity::neighboursAndEdges()
{
  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  return mp_NeighboursMap;
}


// =====================================================================
// =====================================================================


std::vector<int> PolygonEntity::getOrderedNeighbourOfldIds()
{
  std::vector<int> Ids;

  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  NeighboursMap_t::iterator it = mp_NeighboursMap->begin();
  NeighboursMap_t::iterator ite = mp_NeighboursMap->end();

  for (; it != ite; ++it)
  {
    Ids.push_back(it->first->getOfldId());
  }

  std::sort(Ids.begin(), Ids.end());

  return Ids;
}


// =====================================================================
// =====================================================================


void PolygonEntity::computeNeighbours()
{
  delete mp_Neighbours;
  mp_Neighbours = new std::set<LandREntity*>;

  delete mp_NeighboursMap;
  mp_NeighboursMap = new NeighboursMap_t();

  unsigned int iEnd=m_PolyEdges.size();
  for (unsigned int i = 0; i < iEnd; i++)
  {
    PolygonEdge* Edge = m_PolyEdges[i];

    PolygonEntity* OtherFace = nullptr;

    if (Edge->getFaces().size() > 1)
    {
      OtherFace = (Edge->getFaces()[0] == this ? Edge->getFaces()[1] : Edge->getFaces()[0]);
    }

    if (OtherFace)
    {
      ((*mp_NeighboursMap)[OtherFace]).push_back(Edge);
      mp_Neighbours->insert(dynamic_cast<LandREntity*>(OtherFace));
    }
  }

}


// =====================================================================
// =====================================================================


void PolygonEntity::printCurrent()
{
  if (true) return;
  int posj=0;
  for (auto Edge : m_PolyEdges)
  {
    std::cout << "=    Edge " << posj << std::endl;
    std::cout << "=      " << Edge->line()->getCoordinates()->toString() << std::endl;
    posj++;
  }
}

// =====================================================================
// =====================================================================


bool PolygonEntity::isComplete()
{
  std::vector<geos::geom::Geometry*>* Geoms = new std::vector<geos::geom::Geometry*>();
  unsigned int iEnd = m_PolyEdges.size();

  for (unsigned int i = 0; i < iEnd; i++)
  {
    Geoms->push_back(m_PolyEdges.at(i)->line());
  }

  geos::geom::MultiLineString* MLS = geos::geom::GeometryFactory::getDefaultInstance()->createMultiLineString(Geoms);
  geos::geom::LineString* LS =
    LandRTools::computeMergedLineStringFromGeometry(dynamic_cast<geos::geom::Geometry*>(MLS));

  bool Complete = LS && LS->equals(mp_Polygon->getExteriorRing());

  delete MLS;
  delete LS;
  delete Geoms;

  return Complete;
}


// =====================================================================
// =====================================================================


std::vector<PolygonEdge*> PolygonEntity::getCommonEdgesWith(PolygonEntity& Other)
{
  std::vector<PolygonEdge*> Edges;

  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  if (mp_NeighboursMap->count(&Other))
  {
    Edges = mp_NeighboursMap->at(&Other);
  }

  return Edges;
}


// =====================================================================
// =====================================================================


geos::geom::Geometry* PolygonEntity::getBufferedBoundary(double BufferDistance)
{
  return mp_Polygon->getBoundary()->buffer(BufferDistance).release();
}


// =====================================================================
// =====================================================================


void PolygonEntity::computeLineStringNeighbours(LineStringGraph& Graph,
                                                LandRTools::Relationship Relation,
                                                double BufferDistance,
                                                double ContactLength)
{
  if (Relation == LandRTools::TOUCHES && ContactLength == 0)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "ContactLength must be greater than 0 "
                                              "for LandRTools::TOUCHES relationship");
  }
  printCurrent();

  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  delete mp_LineStringNeighboursMap;
  mp_LineStringNeighboursMap = new PolygonEntity::LineStringNeighboursMap_t;

  geos::geom::Geometry* PolyBuff = getBufferedBoundary(BufferDistance);

  openfluid::landr::LandRGraph::Entities_t LSs = Graph.getEntities();

  openfluid::landr::LandRGraph::Entities_t::const_iterator it = LSs.begin();
  openfluid::landr::LandRGraph::Entities_t::const_iterator ite = LSs.end();
  printCurrent();
  for (; it != ite; ++it)
  {
    LineStringEntity* LS = dynamic_cast<LineStringEntity*>(*it);

    // v-- LS->line or PolyBuff is a problem
    if (Relation == LandRTools::CONTAINS && LS->line()->within(PolyBuff))
    {
      geos::geom::Geometry* EdgeBuff;
      unsigned int jEnd=m_PolyEdges.size();

      for (unsigned int j = 0; j < jEnd; j++)
      {
        EdgeBuff = m_PolyEdges[j]->line()->buffer(BufferDistance).release();
        if (LS->line()->within(EdgeBuff))
        {
          mp_LineStringNeighboursMap->insert(
              std::make_pair(LS, m_PolyEdges[j]));
          mp_Neighbours->insert(*it);
        }
        delete EdgeBuff;
      }
    }
    else if (Relation == LandRTools::INTERSECTS && LS->line()->intersects(PolyBuff))
    {
      mp_LineStringNeighboursMap->insert(std::make_pair(LS, (openfluid::landr::PolygonEdge*) 0));
      mp_Neighbours->insert(*it);
    }
    else if (Relation == LandRTools::TOUCHES && LS->line()->intersects(PolyBuff))
    {
      geos::geom::Geometry* EdgeBuff;
      unsigned int jEnd=m_PolyEdges.size();

      for (unsigned int j = 0; j < jEnd; j++)
      {
        EdgeBuff = m_PolyEdges[j]->line()->buffer(BufferDistance).release();

        geos::geom::Geometry* Inter = LS->line()->intersection(EdgeBuff).release();
        double lengthInter = 0.0;
        unsigned int iEnd = Inter->getNumGeometries();

        for (unsigned int i = 0; i < iEnd; i++)
        {
          geos::geom::LineString* LineIntersect =
              dynamic_cast<geos::geom::LineString*>(const_cast<geos::geom::Geometry*>(Inter->getGeometryN(i)));
          lengthInter = lengthInter+LineIntersect->getLength();
        }

        if(lengthInter>ContactLength)
        {
          mp_LineStringNeighboursMap->insert(std::make_pair(LS, m_PolyEdges[j]));
          mp_Neighbours->insert(*it);
        }
        delete EdgeBuff;
      }
    }
  }
  printCurrent();
  delete PolyBuff;
  printCurrent();
}


// =====================================================================
// =====================================================================


PolygonEntity::LineStringNeighboursMap_t* PolygonEntity::lineStringNeighbours()
{
  return mp_LineStringNeighboursMap;
}


// =====================================================================
// =====================================================================


geos::geom::LineString* PolygonEntity::mergeEdges(PolygonEdge* Edge,
                                                  PolygonEdge* EdgeToMerge)
{
  //ensure that the two PolygonEdges are coincident
  if (!Edge->isCoincident(EdgeToMerge))
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"The PolygonEdges are not coincident");
  }

  std::unique_ptr<geos::geom::Point> StartPoint = Edge->line()->getStartPoint();
  std::unique_ptr<geos::geom::Point> EndPoint = Edge->line()->getEndPoint();

  std::unique_ptr<geos::geom::Point> StartPoint2 = EdgeToMerge->line()->getStartPoint();
  std::unique_ptr<geos::geom::Point> EndPoint2 = EdgeToMerge->line()->getEndPoint();

  geos::geom::CoordinateArraySequence CoordsOne;
  geos::geom::CoordinateArraySequence CoordsTwo;


  if (EndPoint->getCoordinate()->equals(*(StartPoint2->getCoordinate())))
  {
    CoordsOne = geos::geom::CoordinateArraySequence(*(Edge->line()->getCoordinates().release()));
    CoordsTwo = geos::geom::CoordinateArraySequence(*(EdgeToMerge->line()->getCoordinates().release()));
    CoordsOne.add(CoordsTwo.clone().release(),false,true);
  }
  else if (StartPoint->getCoordinate()->equals(*(EndPoint2->getCoordinate())))
  {
    CoordsOne = geos::geom::CoordinateArraySequence(*(EdgeToMerge->line()->getCoordinates().release()));
    CoordsTwo = geos::geom::CoordinateArraySequence(*(Edge->line()->getCoordinates().release()));
    CoordsOne.add(CoordsTwo.clone().release(),false,true);
  }
  else if (EndPoint->getCoordinate()->equals(*(EndPoint2->getCoordinate())))
  {
    CoordsOne = geos::geom::CoordinateArraySequence(*(Edge->line()->getCoordinates().release()));
    CoordsTwo = geos::geom::CoordinateArraySequence(*(EdgeToMerge->line()->getCoordinates().release()));
    CoordsOne.add(CoordsTwo.clone().release(),false,false);
  }
  else if (StartPoint->getCoordinate()->equals(*(StartPoint2->getCoordinate())))
  {
    geos::geom::Geometry * reverseLine=EdgeToMerge->line()->reverse().release();

    CoordsOne = geos::geom::CoordinateArraySequence(*(reverseLine->getCoordinates().release()));
    CoordsTwo = geos::geom::CoordinateArraySequence(*(Edge->line()->getCoordinates().release()));
    CoordsOne.add(CoordsTwo.clone().release(),false,true);
    delete reverseLine;
  }

  geos::geom::LineString* NewLine = geos::geom::GeometryFactory::getDefaultInstance()->createLineString(CoordsOne.clone().release());
  return NewLine;
}


// =====================================================================
// =====================================================================


void PolygonEntity::computeNeighboursWithBarriers(LineStringGraph& Graph,
                                                  LandRTools::Relationship Relation,
                                                  double BufferDistance,
                                                  double ContactLength)
{
  if (Relation == LandRTools::TOUCHES && ContactLength==0)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "ContactLength must be greater than 0 "
                                              "for LandRTools::Relationship TOUCHES ");
  }

  if (Relation == LandRTools::INTERSECTS)
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,
                                              "LandRTools::Relationship INTERSECTS is not allowed");
  }

  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  geos::geom::Geometry* PolyBuff = getBufferedBoundary(BufferDistance);
  openfluid::landr::LandRGraph::Entities_t LSs = Graph.getEntities();
  openfluid::landr::LandRGraph::Entities_t::const_iterator it = LSs.begin();
  openfluid::landr::LandRGraph::Entities_t::const_iterator ite = LSs.end();

  for (; it != ite; ++it)
  {
    LineStringEntity* LS = dynamic_cast<LineStringEntity*>(*it);

    if (Relation == LandRTools::CONTAINS && LS->line()->within(PolyBuff))
    {
      geos::geom::Geometry* EdgeBuff;
      unsigned int jEnd=m_PolyEdges.size();

      for (unsigned int j = 0; j < jEnd; j++)
      {
        EdgeBuff = m_PolyEdges[j]->line()->buffer(BufferDistance).release();
        if (LS->line()->within(EdgeBuff))
        {
          // remove from mp_Neighbours and mp_NeighboursMap the polygon which share this Edge
          mp_Neighbours->erase(neighbourWithCommonEdge(m_PolyEdges[j]));
          mp_NeighboursMap->erase(neighbourWithCommonEdge(m_PolyEdges[j]));
        }
        delete EdgeBuff;
      }
    }
    else if (Relation == LandRTools::TOUCHES && LS->line()->intersects(PolyBuff))
    {
      geos::geom::Geometry* EdgeBuff;
      unsigned int jEnd=m_PolyEdges.size();
      for (unsigned int j = 0; j < jEnd; j++)
      {
        EdgeBuff = m_PolyEdges[j]->line()->buffer(BufferDistance).release();
        geos::geom::Geometry * Inter = LS->line()->intersection(EdgeBuff).release();
        double lengthInter = 0.0;
        unsigned int iEnd=Inter->getNumGeometries();

        for (unsigned int i = 0; i < iEnd; i++)
        {
          geos::geom::LineString* LineIntersect =
              dynamic_cast<geos::geom::LineString*>(const_cast<geos::geom::Geometry*>(Inter->getGeometryN(i)));
          lengthInter = lengthInter+LineIntersect->getLength();
        }

        if (lengthInter>ContactLength)
        {
          // remove from mp_Neighbours and mp_NeighboursMap the polygon which share this Edge
          mp_Neighbours->erase(neighbourWithCommonEdge(m_PolyEdges[j]));
          mp_NeighboursMap->erase(neighbourWithCommonEdge(m_PolyEdges[j]));
        }
        delete EdgeBuff;
      }
    }
  }

  delete PolyBuff;
}


// =====================================================================
// =====================================================================


PolygonEntity * PolygonEntity::neighbourWithCommonEdge(PolygonEdge * Edge)
{
  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  NeighboursMap_t::iterator it = mp_NeighboursMap->begin();
  NeighboursMap_t::iterator ite = mp_NeighboursMap->end();

  for (; it != ite; ++it)
  {
    std::vector<PolygonEdge*> vEdges = (*it).second;
    std::vector<PolygonEdge*>::iterator jt;

    jt = find (vEdges.begin(), vEdges.end(), Edge);
    if (jt != vEdges.end())
    {
      return (*it).first;
    }
  }
  return nullptr;
}


// =====================================================================
// =====================================================================


std::multimap<double,PolygonEntity*> PolygonEntity::getOrderedNeighboursByLengthBoundary()
{

  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  PolygonEntity::NeighboursMap_t::iterator it = mp_NeighboursMap->begin();
  PolygonEntity::NeighboursMap_t::iterator ite = mp_NeighboursMap->end();
  std::multimap<double, PolygonEntity*> mNeighbours;

  for (; it != ite; ++it)
  {
    std::vector<PolygonEdge*>::iterator jt = it->second.begin();
    std::vector<PolygonEdge*>::iterator jte = it->second.end();
    double length = 0.0;

    for (; jt != jte; ++jt)
    {
      length += (*jt)->line()->getLength();
    }

    mNeighbours.insert (std::pair<double,PolygonEntity*>(length,&(*it->first) ));
  }

  return mNeighbours;
}


// =====================================================================
// =====================================================================


std::pair<LandREntity *, double> PolygonEntity::computeNeighbourByLineTopology(VectorDataset LineTopology)
{
  if (!LineTopology.isLineType())
  {
    throw openfluid::base::FrameworkException(OPENFLUID_CODE_LOCATION,"The VectorDataset is not Line Type ");
  }

  if (!mp_NeighboursMap)
  {
    computeNeighbours();
  }

  openfluid::landr::LandREntity* Down = nullptr;
  double FlowLength = 0.0;

  std::pair<LandREntity*, double > pairNeighLength;

  // no Intersection between this PolygonEntity and the lines of the VectorDataset
  if (!(geometry()->intersects(LineTopology.geometries())))
  {
    pairNeighLength = std::make_pair(Down,FlowLength);
    return pairNeighLength;
  }

  geos::geom::Geometry* PtIntersect = (LineTopology.geometries());
  geos::geom::LineString* Line = nullptr;
  bool cover = false;
  unsigned int i = 0;

  while (i<PtIntersect->getNumGeometries()&&!cover)
  {
    const geos::geom::Geometry* GeomL = PtIntersect->getGeometryN(i);
    Line = dynamic_cast<geos::geom::LineString*>(const_cast<geos::geom::Geometry*>(GeomL));
    std::unique_ptr<geos::geom::Point> Point = Line->getStartPoint();

    if (geometry()->covers(Point.get()))
    {
      cover=true;
    }
    else
    {
      i++;
    }
  }

  // no Intersection between this PolygonEntity and the lines of the VectorDataset
  if (!cover)
  {
    pairNeighLength = std::make_pair(Down,FlowLength);
    return pairNeighLength;
  }

  std::unique_ptr<geos::geom::Point> EndPoint = Line->getEndPoint();
  bool downUnit = false;

  NeighboursMap_t::iterator jt = mp_NeighboursMap->begin();
  NeighboursMap_t::iterator jte = mp_NeighboursMap->end();

  while (jt != jte&&!downUnit)
  {
    if (((*jt).first->geometry())->covers(EndPoint.get()) && (getOfldId()!=(*jt).first->getOfldId()))
    {
      Down = (*jt).first;
      downUnit = true;
      FlowLength = Line->getLength();
    }
    else
    {
      ++jt;
    }
  }

  // check if this PolygonEntity has LineString neighbours and if Line intersects a LineString Neighbour

  if (mp_LineStringNeighboursMap)
  {
    LineStringNeighboursMap_t::iterator ht = mp_LineStringNeighboursMap->begin();
    LineStringNeighboursMap_t::iterator hte = mp_LineStringNeighboursMap->end();
    downUnit = false;

    while (ht != hte&&!downUnit)
    {
      if ((*ht).first->line()->intersects(Line))
      {
        Down = (*ht).first;
        downUnit = true;
        geos::geom::Geometry *Intersect = Line->difference((*ht).first->line()).release();
        unsigned int hEnd=Intersect->getNumGeometries();

        for (unsigned int h = 0; h < hEnd; h++)
        {
          geos::geom::LineString* LinePart =
            (dynamic_cast<geos::geom::LineString*>(const_cast<geos::geom::Geometry*>(Intersect->getGeometryN(h))));

          // FLowLength is the length of the line which EndPoint touches the LineString Neighbour
          if (LinePart->getEndPoint()->isWithinDistance((*ht).first->line(),0.0001) &&
              geometry()->covers(LinePart->getStartPoint().get()))
          {
            FlowLength=LinePart->getLength();
          }

          delete LinePart;
        }
      }
      else
      {
        ++ht;
      }
    }
  }

  pairNeighLength = std::make_pair(Down,FlowLength);

  return pairNeighLength;
}


} } // namespaces
