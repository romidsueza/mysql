/* Copyright (C) 2003-2006 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


/**
  @file

  @brief
  This file defines all spatial functions
*/

#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation				// gcc: Class implementation
#endif

#include "sql_priv.h"
/*
  It is necessary to include set_var.h instead of item.h because there
  are dependencies on include order for set_var.h and item.h. This
  will be resolved later.
*/
#include "sql_class.h"                          // THD, set_var.h: THD
#include "set_var.h"
#ifdef HAVE_SPATIAL
#include <m_ctype.h>

const double EPSILON= 1e-8;

Field *Item_geometry_func::tmp_table_field(TABLE *t_arg)
{
  Field *result;
  if ((result= new Field_geom(max_length, maybe_null, name, t_arg->s,
                              get_geometry_type())))
    result->init(t_arg);
  return result;
}

void Item_geometry_func::fix_length_and_dec()
{
  collation.set(&my_charset_bin);
  decimals=0;
  max_length= max_field_size;
  maybe_null= 1;
}

bool Item_geometry_func::send(Protocol *protocol, String *str)
{
  return Item_str_func::send(protocol, str);
/*
  String *res;
  if ((res=val_str(str)) == NULL)
    return protocol->store_null();

    result= protocol->store(res->ptr(),res->length(),res->charset());
  return result;
*/
}


String *Item_func_geometry_from_text::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  Geometry_buffer buffer;
  String arg_val;
  String *wkt= args[0]->val_str(&arg_val);

  if ((null_value= args[0]->null_value))
    return 0;

  Gis_read_stream trs(wkt->charset(), wkt->ptr(), wkt->length());
  uint32 srid= 0;

  if ((arg_count == 2) && !args[1]->null_value)
    srid= (uint32)args[1]->val_int();

  str->set_charset(&my_charset_bin);
  if (str->reserve(SRID_SIZE, 512))
    return 0;
  str->length(0);
  str->q_append(srid);
  if ((null_value= !Geometry::create_from_wkt(&buffer, &trs, str, 0)))
    return 0;
  return str;
}


String *Item_func_geometry_from_wkb::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *wkb;
  Geometry_buffer buffer;
  uint32 srid= 0;

  if (args[0]->field_type() == MYSQL_TYPE_GEOMETRY)
  {
    String *str_ret= args[0]->val_str(str);
    null_value= args[0]->null_value;
    return str_ret;
  }

  wkb= args[0]->val_str(&arg_val);

  if ((arg_count == 2) && !args[1]->null_value)
    srid= (uint32)args[1]->val_int();

  str->set_charset(&my_charset_bin);
  if (str->reserve(SRID_SIZE, 512))
  {
    null_value= TRUE;                           /* purecov: inspected */
    return 0;                                   /* purecov: inspected */
  }
  str->length(0);
  str->q_append(srid);
  if ((null_value= 
        (args[0]->null_value ||
         !Geometry::create_from_wkb(&buffer, wkb->ptr(), wkb->length(), str))))
    return 0;
  return str;
}


String *Item_func_as_wkt::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *swkb= args[0]->val_str(&arg_val);
  Geometry_buffer buffer;
  Geometry *geom= NULL;
  const char *dummy;

  if ((null_value=
       (args[0]->null_value ||
	!(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length())))))
    return 0;

  str->length(0);
  if ((null_value= geom->as_wkt(str, &dummy)))
    return 0;

  return str;
}


void Item_func_as_wkt::fix_length_and_dec()
{
  max_length=MAX_BLOB_WIDTH;
  maybe_null= 1;
}


String *Item_func_as_wkb::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *swkb= args[0]->val_str(&arg_val);
  Geometry_buffer buffer;

  if ((null_value=
       (args[0]->null_value ||
	!(Geometry::construct(&buffer, swkb->ptr(), swkb->length())))))
    return 0;

  str->copy(swkb->ptr() + SRID_SIZE, swkb->length() - SRID_SIZE,
	    &my_charset_bin);
  return str;
}


String *Item_func_geometry_type::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String *swkb= args[0]->val_str(str);
  Geometry_buffer buffer;
  Geometry *geom= NULL;

  if ((null_value=
       (args[0]->null_value ||
	!(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length())))))
    return 0;
  /* String will not move */
  str->copy(geom->get_class_info()->m_name.str,
	    geom->get_class_info()->m_name.length,
	    default_charset());
  return str;
}


Field::geometry_type Item_func_envelope::get_geometry_type() const
{
  return Field::GEOM_POLYGON;
}


String *Item_func_envelope::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *swkb= args[0]->val_str(&arg_val);
  Geometry_buffer buffer;
  Geometry *geom= NULL;
  uint32 srid;
  
  if ((null_value=
       args[0]->null_value ||
       !(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length()))))
    return 0;
  
  srid= uint4korr(swkb->ptr());
  str->set_charset(&my_charset_bin);
  str->length(0);
  if (str->reserve(SRID_SIZE, 512))
    return 0;
  str->q_append(srid);
  return (null_value= geom->envelope(str)) ? 0 : str;
}


Field::geometry_type Item_func_centroid::get_geometry_type() const
{
  return Field::GEOM_POINT;
}


String *Item_func_centroid::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *swkb= args[0]->val_str(&arg_val);
  Geometry_buffer buffer;
  Geometry *geom= NULL;
  uint32 srid;

  if ((null_value= args[0]->null_value ||
       !(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length()))))
    return 0;

  str->set_charset(&my_charset_bin);
  if (str->reserve(SRID_SIZE, 512))
    return 0;
  str->length(0);
  srid= uint4korr(swkb->ptr());
  str->q_append(srid);

  return (null_value= test(geom->centroid(str))) ? 0 : str;
}


/*
  Spatial decomposition functions
*/

String *Item_func_spatial_decomp::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *swkb= args[0]->val_str(&arg_val);
  Geometry_buffer buffer;
  Geometry *geom= NULL;
  uint32 srid;

  if ((null_value=
       (args[0]->null_value ||
	!(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length())))))
    return 0;

  srid= uint4korr(swkb->ptr());
  str->set_charset(&my_charset_bin);
  if (str->reserve(SRID_SIZE, 512))
    goto err;
  str->length(0);
  str->q_append(srid);
  switch (decomp_func) {
    case SP_STARTPOINT:
      if (geom->start_point(str))
        goto err;
      break;

    case SP_ENDPOINT:
      if (geom->end_point(str))
        goto err;
      break;

    case SP_EXTERIORRING:
      if (geom->exterior_ring(str))
        goto err;
      break;

    default:
      goto err;
  }
  return str;

err:
  null_value= 1;
  return 0;
}


String *Item_func_spatial_decomp_n::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_val;
  String *swkb= args[0]->val_str(&arg_val);
  long n= (long) args[1]->val_int();
  Geometry_buffer buffer;
  Geometry *geom= NULL;
  uint32 srid;

  if ((null_value=
       (args[0]->null_value || args[1]->null_value ||
	!(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length())))))
    return 0;

  str->set_charset(&my_charset_bin);
  if (str->reserve(SRID_SIZE, 512))
    goto err;
  srid= uint4korr(swkb->ptr());
  str->length(0);
  str->q_append(srid);
  switch (decomp_func_n)
  {
    case SP_POINTN:
      if (geom->point_n(n,str))
        goto err;
      break;

    case SP_GEOMETRYN:
      if (geom->geometry_n(n,str))
        goto err;
      break;

    case SP_INTERIORRINGN:
      if (geom->interior_ring_n(n,str))
        goto err;
      break;

    default:
      goto err;
  }
  return str;

err:
  null_value=1;
  return 0;
}


/*
  Functions to concatenate various spatial objects
*/


/*
*  Concatenate doubles into Point
*/


Field::geometry_type Item_func_point::get_geometry_type() const
{
  return Field::GEOM_POINT;
}


String *Item_func_point::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  double x= args[0]->val_real();
  double y= args[1]->val_real();
  uint32 srid= 0;

  if ((null_value= (args[0]->null_value ||
		    args[1]->null_value ||
                    str->realloc(4/*SRID*/ + 1 + 4 + SIZEOF_STORED_DOUBLE*2))))
    return 0;

  str->set_charset(&my_charset_bin);
  str->length(0);
  str->q_append(srid);
  str->q_append((char)Geometry::wkb_ndr);
  str->q_append((uint32)Geometry::wkb_point);
  str->q_append(x);
  str->q_append(y);
  return str;
}


/**
  Concatenates various items into various collections
  with checkings for valid wkb type of items.
  For example, MultiPoint can be a collection of Points only.
  coll_type contains wkb type of target collection.
  item_type contains a valid wkb type of items.
  In the case when coll_type is wkbGeometryCollection,
  we do not check wkb type of items, any is valid.
*/

String *Item_func_spatial_collection::val_str(String *str)
{
  DBUG_ASSERT(fixed == 1);
  String arg_value;
  uint i;
  uint32 srid= 0;

  str->set_charset(&my_charset_bin);
  str->length(0);
  if (str->reserve(4/*SRID*/ + 1 + 4 + 4, 512))
    goto err;

  str->q_append(srid);
  str->q_append((char) Geometry::wkb_ndr);
  str->q_append((uint32) coll_type);
  str->q_append((uint32) arg_count);

  for (i= 0; i < arg_count; ++i)
  {
    String *res= args[i]->val_str(&arg_value);
    uint32 len;
    if (args[i]->null_value || ((len= res->length()) < WKB_HEADER_SIZE))
      goto err;

    if (coll_type == Geometry::wkb_geometrycollection)
    {
      /*
	In the case of GeometryCollection we don't need any checkings
	for item types, so just copy them into target collection
      */
      if (str->append(res->ptr() + 4/*SRID*/, len - 4/*SRID*/, (uint32) 512))
        goto err;
    }
    else
    {
      enum Geometry::wkbType wkb_type;
      const uint data_offset= 4/*SRID*/ + 1;
      if (res->length() < data_offset + sizeof(uint32))
        goto err;
      const char *data= res->ptr() + data_offset;

      /*
	In the case of named collection we must check that items
	are of specific type, let's do this checking now
      */

      wkb_type= (Geometry::wkbType) uint4korr(data);
      data+= 4;
      len-= 5 + 4/*SRID*/;
      if (wkb_type != item_type)
        goto err;

      switch (coll_type) {
      case Geometry::wkb_multipoint:
      case Geometry::wkb_multilinestring:
      case Geometry::wkb_multipolygon:
	if (len < WKB_HEADER_SIZE ||
	    str->append(data-WKB_HEADER_SIZE, len+WKB_HEADER_SIZE, 512))
	  goto err;
	break;

      case Geometry::wkb_linestring:
	if (len < POINT_DATA_SIZE || str->append(data, POINT_DATA_SIZE, 512))
	  goto err;
	break;
      case Geometry::wkb_polygon:
      {
	uint32 n_points;
	double x1, y1, x2, y2;
	const char *org_data= data;

	if (len < 4)
	  goto err;

	n_points= uint4korr(data);
	data+= 4;

        if (n_points < 2 || len < 4 + n_points * POINT_DATA_SIZE)
          goto err;
        
	float8get(x1, data);
	data+= SIZEOF_STORED_DOUBLE;
	float8get(y1, data);
	data+= SIZEOF_STORED_DOUBLE;

	data+= (n_points - 2) * POINT_DATA_SIZE;

	float8get(x2, data);
	float8get(y2, data + SIZEOF_STORED_DOUBLE);

	if ((x1 != x2) || (y1 != y2) ||
	    str->append(org_data, len, 512))
	  goto err;
      }
      break;

      default:
	goto err;
      }
    }
  }
  if (str->length() > current_thd->variables.max_allowed_packet)
  {
    push_warning_printf(current_thd, MYSQL_ERROR::WARN_LEVEL_WARN,
			ER_WARN_ALLOWED_PACKET_OVERFLOWED,
			ER(ER_WARN_ALLOWED_PACKET_OVERFLOWED),
			func_name(), current_thd->variables.max_allowed_packet);
    goto err;
  }

  null_value = 0;
  return str;

err:
  null_value= 1;
  return 0;
}


/*
  Functions for spatial relations
*/

const char *Item_func_spatial_mbr_rel::func_name() const 
{ 
  switch (spatial_rel) {
    case SP_CONTAINS_FUNC:
      return "mbrcontains";
    case SP_WITHIN_FUNC:
      return "mbrwithin";
    case SP_EQUALS_FUNC:
      return "mbrequals";
    case SP_DISJOINT_FUNC:
      return "mbrdisjoint";
    case SP_INTERSECTS_FUNC:
      return "mbrintersects";
    case SP_TOUCHES_FUNC:
      return "mbrtouches";
    case SP_CROSSES_FUNC:
      return "mbrcrosses";
    case SP_OVERLAPS_FUNC:
      return "mbroverlaps";
    default:
      DBUG_ASSERT(0);  // Should never happened
      return "mbrsp_unknown"; 
  }
}


longlong Item_func_spatial_mbr_rel::val_int()
{
  DBUG_ASSERT(fixed == 1);
  String *res1= args[0]->val_str(&cmp.value1);
  String *res2= args[1]->val_str(&cmp.value2);
  Geometry_buffer buffer1, buffer2;
  Geometry *g1, *g2;
  MBR mbr1, mbr2;
  const char *dummy;

  if ((null_value=
       (args[0]->null_value ||
	args[1]->null_value ||
	!(g1= Geometry::construct(&buffer1, res1->ptr(), res1->length())) ||
	!(g2= Geometry::construct(&buffer2, res2->ptr(), res2->length())) ||
	g1->get_mbr(&mbr1, &dummy) ||
	g2->get_mbr(&mbr2, &dummy))))
   return 0;

  switch (spatial_rel) {
    case SP_CONTAINS_FUNC:
      return mbr1.contains(&mbr2);
    case SP_WITHIN_FUNC:
      return mbr1.within(&mbr2);
    case SP_EQUALS_FUNC:
      return mbr1.equals(&mbr2);
    case SP_DISJOINT_FUNC:
      return mbr1.disjoint(&mbr2);
    case SP_INTERSECTS_FUNC:
      return mbr1.intersects(&mbr2);
    case SP_TOUCHES_FUNC:
      return mbr1.touches(&mbr2);
    case SP_OVERLAPS_FUNC:
      return mbr1.overlaps(&mbr2);
    case SP_CROSSES_FUNC:
      return 0;
    default:
      break;
  }

  null_value=1;
  return 0;
}


Item_func_spatial_rel::Item_func_spatial_rel(Item *a,Item *b,
                                             enum Functype sp_rel) :
    Item_bool_func2(a,b), collector()
{
  spatial_rel = sp_rel;
}


Item_func_spatial_rel::~Item_func_spatial_rel()
{
}


const char *Item_func_spatial_rel::func_name() const 
{ 
  switch (spatial_rel) {
    case SP_CONTAINS_FUNC:
      return "st_contains";
    case SP_WITHIN_FUNC:
      return "st_within";
    case SP_EQUALS_FUNC:
      return "st_equals";
    case SP_DISJOINT_FUNC:
      return "st_disjoint";
    case SP_INTERSECTS_FUNC:
      return "st_intersects";
    case SP_TOUCHES_FUNC:
      return "st_touches";
    case SP_CROSSES_FUNC:
      return "st_crosses";
    case SP_OVERLAPS_FUNC:
      return "st_overlaps";
    default:
      DBUG_ASSERT(0);  // Should never happened
      return "sp_unknown"; 
  }
}


static double count_edge_t(const gcalc_heap::info *ea,
                           const gcalc_heap::info *eb,
                           const gcalc_heap::info *v,
                           double &ex, double &ey, double &vx, double &vy,
                           double &e_sqrlen)
{
  ex= eb->x - ea->x;
  ey= eb->y - ea->y;
  vx= v->x - ea->x;
  vy= v->y - ea->y;
  e_sqrlen= ex*ex + ey*ey;
  return (ex*vx + ey*vy) / e_sqrlen;
}


static double distance_to_line(double ex, double ey, double vx, double vy,
                               double e_sqrlen)
{
  return fabs(vx*ey - vy*ex) / sqrt(e_sqrlen);
}


static double distance_points(const gcalc_heap::info *a,
                              const gcalc_heap::info *b)
{
  double x= a->x - b->x;
  double y= a->y - b->y;
  return sqrt(x*x + y*y);
}


/*
  Calculates the distance between objects.
*/

static int calc_distance(double *result, gcalc_heap *collector, int obj2_si,
                         gcalc_function *func, gcalc_scan_iterator *scan_it)
{
  bool above_cur_point, cur_point_edge;
  const gcalc_scan_iterator::point *evpos;
  const gcalc_heap::info *cur_point, *dist_point;
  gcalc_scan_events ev;
  double t, distance, cur_distance;
  double ex, ey, vx, vy, e_sqrlen;

  DBUG_ENTER("calc_distance");

  above_cur_point= false;
  distance= DBL_MAX;

  while (scan_it->more_points())
  {
    if (scan_it->step())
      goto mem_error;
    evpos= scan_it->get_event_position();
    ev= scan_it->get_event();
    cur_point= evpos->pi;

    /*
       handling intersection we only need to check if it's the intersecion
       of objects 1 and 2. In this case distance is 0
    */
    if (ev == scev_intersection)
    {
      if ((evpos->get_next()->pi->shape >= obj2_si) !=
            (cur_point->shape >= obj2_si))
      {
        distance= 0;
        goto exit;
      }
      continue;
    }

    /*
       if we get 'scev_point | scev_end | scev_two_ends' we don't need
       to check for intersection of objects.
       Though we need to calculate distances.
    */
    if (ev & (scev_point | scev_end | scev_two_ends))
      goto calculate_distance;

    goto calculate_distance;
    /*
       having these events we need to check for possible intersection
       of objects
       scev_thread | scev_two_threads | scev_single_point
    */
    DBUG_ASSERT(ev & (scev_thread | scev_two_threads | scev_single_point));

    func->clear_state();
    for (gcalc_point_iterator pit(scan_it); pit.point() != evpos; ++pit)
    {
      gcalc_shape_info si= pit.point()->get_shape();
      if ((func->get_shape_kind(si) == gcalc_function::shape_polygon))
        func->invert_state(si);
    }
    func->invert_state(evpos->get_shape());
    if (func->count())
    {
      /* Point of one object is inside the other - intersection found */
      distance= 0;
      goto exit;
    }


calculate_distance:
    if (cur_point->shape >= obj2_si)
      continue;
    cur_point_edge= !cur_point->is_bottom();

    for (dist_point= collector->get_first(); dist_point; dist_point= dist_point->get_next())
    {
      /* We only check vertices of object 2 */
      if (dist_point->shape < obj2_si)
        continue;

      /* if we have an edge to check */
      if (dist_point->left)
      {
        t= count_edge_t(dist_point, dist_point->left, cur_point,
                        ex, ey, vx, vy, e_sqrlen);
        if ((t>0.0) && (t<1.0))
        {
          cur_distance= distance_to_line(ex, ey, vx, vy, e_sqrlen);
          if (distance > cur_distance)
            distance= cur_distance;
        }
      }
      if (cur_point_edge)
      {
        t= count_edge_t(cur_point, cur_point->left, dist_point,
                        ex, ey, vx, vy, e_sqrlen);
        if ((t>0.0) && (t<1.0))
        {
          cur_distance= distance_to_line(ex, ey, vx, vy, e_sqrlen);
          if (distance > cur_distance)
            distance= cur_distance;
        }
      }
      cur_distance= distance_points(cur_point, dist_point);
      if (distance > cur_distance)
        distance= cur_distance;
    }
  }

exit:
  *result= distance;
  DBUG_RETURN(0);

mem_error:
  DBUG_RETURN(1);
}


int Item_func_spatial_rel::func_touches()
{
  bool above_cur_point, cur_point_edge;
  double x1, x2, y1, y2, ex, ey;
  double distance, area;
  int result= 0;
  int cur_func= 0;

  gcalc_operation_transporter trn(&func, &collector);

  String *res1= args[0]->val_str(&tmp_value1);
  String *res2= args[1]->val_str(&tmp_value2);
  Geometry_buffer buffer1, buffer2;
  Geometry *g1, *g2;
  int obj2_si;

  DBUG_ENTER("Item_func_spatial_rel::func_touches");
  DBUG_ASSERT(fixed == 1);

  if ((null_value= (args[0]->null_value || args[1]->null_value ||
          !(g1= Geometry::construct(&buffer1, res1->ptr(), res1->length())) ||
          !(g2= Geometry::construct(&buffer2, res2->ptr(), res2->length())))))
    goto mem_error;

  if ((g1->get_class_info()->m_type_id == Geometry::wkb_point) &&
      (g2->get_class_info()->m_type_id == Geometry::wkb_point))
  {
    if (((Gis_point *)g1)->get_xy(&x1, &y1) ||
        ((Gis_point *)g2)->get_xy(&x2, &y2))
      goto mem_error;
    ex= x2 - x1;
    ey= y2 - y1;
    DBUG_RETURN((ex * ex + ey * ey) < EPSILON);
  }

  if (func.reserve_op_buffer(1))
    goto mem_error;
  func.add_operation(gcalc_function::op_intersection, 2);

  if (g1->store_shapes(&trn))
    goto mem_error;
  obj2_si= func.get_nshapes();

  if (g2->store_shapes(&trn) || func.alloc_states())
    goto mem_error;

  collector.prepare_operation();
  scan_it.init(&collector);

  if (calc_distance(&distance, &collector, obj2_si, &func, &scan_it))
   goto mem_error;
  if (distance > EPSILON)
    goto exit;

  scan_it.reset();
  scan_it.init(&collector);

  above_cur_point= false;
  distance= DBL_MAX;

  while (scan_it.more_trapeziums())
  {
    if (scan_it.step())
      goto mem_error;

    func.clear_state();
    for (gcalc_trapezium_iterator ti(&scan_it); ti.more(); ++ti)
    {
      gcalc_shape_info si= ti.lb()->get_shape();
      if ((func.get_shape_kind(si) == gcalc_function::shape_polygon))
      {
        func.invert_state(si);
        cur_func= func.count();
      }
      if (cur_func)
      {
        area= scan_it.get_h() *
              ((ti.rb()->x - ti.lb()->x) + (ti.rt()->x - ti.lt()->x));
        if (area > EPSILON)
        {
          result= 0;
          goto exit;
        }
      }
    }
  }
  result= 1;

exit:
  collector.reset();
  func.reset();
  scan_it.reset();
  DBUG_RETURN(result);
mem_error:
  null_value= 1;
  DBUG_RETURN(0);
}


longlong Item_func_spatial_rel::val_int()
{
  DBUG_ENTER("Item_func_spatial_rel::val_int");
  DBUG_ASSERT(fixed == 1);
  String *res1;
  String *res2;
  Geometry_buffer buffer1, buffer2;
  Geometry *g1, *g2;
  int result= 0;
  int mask= 0;

  if (spatial_rel == SP_TOUCHES_FUNC)
    DBUG_RETURN(func_touches());

  res1= args[0]->val_str(&tmp_value1);
  res2= args[1]->val_str(&tmp_value2);
  gcalc_operation_transporter trn(&func, &collector);

  if (func.reserve_op_buffer(1))
    DBUG_RETURN(0);

  switch (spatial_rel) {
    case SP_CONTAINS_FUNC:
      mask= 1;
      func.add_operation(gcalc_function::op_backdifference, 2);
      break;
    case SP_WITHIN_FUNC:
      mask= 1;
      func.add_operation(gcalc_function::op_difference, 2);
      break;
    case SP_EQUALS_FUNC:
      goto exit;
    case SP_DISJOINT_FUNC:
      mask= 1;
      func.add_operation(gcalc_function::op_intersection, 2);
      break;
    case SP_INTERSECTS_FUNC:
      func.add_operation(gcalc_function::op_intersection, 2);
      break;
    case SP_OVERLAPS_FUNC:
      func.add_operation(gcalc_function::op_intersection, 2);
      break;
    case SP_CROSSES_FUNC:
      func.add_operation(gcalc_function::op_intersection, 2);
      break;
    default:
      DBUG_ASSERT(FALSE);
      break;
  }


  if ((null_value=
       (args[0]->null_value || args[1]->null_value ||
	!(g1= Geometry::construct(&buffer1, res1->ptr(), res1->length())) ||
	!(g2= Geometry::construct(&buffer2, res2->ptr(), res2->length())) ||
	g1->store_shapes(&trn) || g2->store_shapes(&trn))))
    goto exit;

  collector.prepare_operation();
  scan_it.init(&collector);
  if (func.alloc_states())
    goto exit;

  result= func.find_function(scan_it) ^ mask;

exit:
  collector.reset();
  func.reset();
  scan_it.reset();
  DBUG_RETURN(result);
}


Item_func_spatial_operation::
  Item_func_spatial_operation(Item *a,Item *b, gcalc_function::op_type sp_op) :
    Item_geometry_func(a, b), spatial_op(sp_op)
{
}


Item_func_spatial_operation::~Item_func_spatial_operation()
{
}


String *Item_func_spatial_operation::val_str(String *str_value)
{
  DBUG_ENTER("Item_func_spatial_operation::val_str");
  DBUG_ASSERT(fixed == 1);
  String *res1= args[0]->val_str(&tmp_value1);
  String *res2= args[1]->val_str(&tmp_value2);
  Geometry_buffer buffer1, buffer2;
  Geometry *g1, *g2;
  uint32 srid= 0;
  gcalc_operation_transporter trn(&func, &collector);

  if (func.reserve_op_buffer(1))
    DBUG_RETURN(0);
  func.add_operation(spatial_op, 2);

  if ((null_value=
       (args[0]->null_value || args[1]->null_value ||
	!(g1= Geometry::construct(&buffer1, res1->ptr(), res1->length())) ||
	!(g2= Geometry::construct(&buffer2, res2->ptr(), res2->length())) ||
	g1->store_shapes(&trn) || g2->store_shapes(&trn))))
    goto exit;

  
  GCALC_DBUG_STARTFILE("/home/hf/gcalc_log");

  collector.prepare_operation();
  scan_it.init(&collector);
  if (func.alloc_states())
    goto exit;

  operation.init(&func);

  if (operation.count_all(&collector) ||
      operation.get_result(&res_receiver))
    goto exit;


  str_value->set_charset(&my_charset_bin);
  if (str_value->reserve(SRID_SIZE, 512))
    goto exit;
  str_value->length(0);
  str_value->q_append(srid);

  if (!Geometry::create_from_opresult(&buffer1, str_value, res_receiver))
    goto exit;

exit:
  GCALC_DBUG_STOP;
  collector.reset();
  func.reset();
  scan_it.reset();
  res_receiver.reset();
  DBUG_RETURN(str_value);
}


const char *Item_func_spatial_operation::func_name() const
{ 
  switch (spatial_op) {
    case gcalc_function::op_intersection:
      return "ST_Intersection";
    case gcalc_function::op_difference:
      return "ST_Difference";
    case gcalc_function::op_union:
      return "ST_Union";
    case gcalc_function::op_symdifference:
      return "ST_Symdifference";
    default:
      DBUG_ASSERT(0);  // Should never happen
      return "sp_unknown"; 
  }
}


static double n_sinus[33]=
{
  0,
  0.04906767432741802,
  0.0980171403295606,
  0.1467304744553618,
  0.1950903220161283,
  0.2429801799032639,
  0.2902846772544623,
  0.3368898533922201,
  0.3826834323650898,
  0.4275550934302821,
  0.4713967368259976,
  0.5141027441932217,
  0.5555702330196022,
  0.5956993044924334,
  0.6343932841636455,
  0.6715589548470183,
  0.7071067811865475,
  0.7409511253549591,
  0.773010453362737,
  0.8032075314806448,
  0.8314696123025452,
  0.8577286100002721,
  0.8819212643483549,
  0.9039892931234433,
  0.9238795325112867,
  0.9415440651830208,
  0.9569403357322089,
  0.970031253194544,
  0.9807852804032304,
  0.989176509964781,
  0.9951847266721968,
  0.9987954562051724,
  1
};


static void get_n_sincos(int n, double *sinus, double *cosinus)
{
  if (n < 33)
  {
    *sinus= n_sinus[n];
    *cosinus= n_sinus[32-n];
  }
  else
  {
    n-= 32;
    *sinus= n_sinus[32-n];
    *cosinus= -n_sinus[n];
  }
}


static int fill_half_circle(gcalc_shape_transporter *trn, double x, double y,
                            double ax, double ay)
{
  double n_sin, n_cos;
  double x_n, y_n;
  for (int n=1; n<63; n++)
  {
    get_n_sincos(n, &n_sin, &n_cos);
    x_n= ax * n_cos - ay * n_sin;
    y_n= ax * n_sin + ay * n_cos;
    if (trn->add_point(x_n + x, y_n + y))
      return 1;
  }
  return 0;
}


#define GIS_ZERO 0.00000000001

static int fill_gap(gcalc_shape_transporter *trn,
                    double x, double y,
                    double ax, double ay, double bx, double by, double d,
                    bool *empty_gap)
{
  double ab= ax*bx + ay*by;
  double cosab= ab / (d*d) + GIS_ZERO;
  double n_sin, n_cos;
  double x_n, y_n;
  int n=1;

  *empty_gap= true;
  for (;;)
  {
    get_n_sincos(n++, &n_sin, &n_cos);
    if (n_cos <= cosab)
      break;
    *empty_gap= false;
    x_n= ax * n_cos - ay * n_sin;
    y_n= ax * n_sin + ay * n_cos;
    if (trn->add_point(x_n + x, y_n + y))
      return 1;
  }
  return 0;
}


/*
  Calculates the vector (p2,p1) and
  negatively orthogonal to it with the length of d.
  The result is (ex,ey) - the vector, (px,py) - the orthogonal.
*/

static void calculate_perpendicular(const gcalc_heap::info *p1,
                                    const gcalc_heap::info *p2, double d,
                                    double *ex, double *ey,
                                    double *px, double *py)
{
  double q;
  *ex= p1->x - p2->x;
  *ey= p1->y - p2->y;
  q= d / sqrt((*ex) * (*ex) + (*ey) * (*ey));
  *px= (*ey) * q;
  *py= -(*ex) * q;
}


static void calculate_perpendicular(
    double x1, double y1, double x2, double y2, double d,
    double *ex, double *ey,
    double *px, double *py)
{
  double q;
  *ex= x1 - x2;
  *ey= y1 - y2;
  q= d / sqrt((*ex) * (*ex) + (*ey) * (*ey));
  *px= (*ey) * q;
  *py= -(*ex) * q;
}


int Item_func_buffer::transporter::single_point(double x, double y)
{
  return add_point_buffer(x, y);
}


int Item_func_buffer::transporter::add_edge_buffer(
  double x3, double y3, bool round_p1, bool round_p2)
{
  gcalc_operation_transporter trn(m_fn, m_heap);
  double e1_x, e1_y, e2_x, e2_y, p1_x, p1_y, p2_x, p2_y;
  double e1e2;
  double sin1, cos1;
  double x_n, y_n;
  bool empty_gap1, empty_gap2;

  ++m_nshapes;
  if (trn.start_simple_poly())
    return 1;

  calculate_perpendicular(x1, y1, x2, y2, m_d, &e1_x, &e1_y, &p1_x, &p1_y);
  calculate_perpendicular(x3, y3, x2, y2, m_d, &e2_x, &e2_y, &p2_x, &p2_y);

  e1e2= e1_x * e2_y - e2_x * e1_y;
  sin1= n_sinus[1];
  cos1= n_sinus[31];
  if (e1e2 < 0)
  {
    empty_gap2= false;
    x_n= x2 + p2_x * cos1 - p2_y * sin1;
    y_n= y2 + p2_y * cos1 + p2_x * sin1;
    if (fill_gap(&trn, x2, y2, -p1_x,-p1_y, p2_x,p2_y, m_d, &empty_gap1) ||
        trn.add_point(x2 + p2_x, y2 + p2_y) ||
        trn.add_point(x_n, y_n))
      return 1;
  }
  else
  {
    x_n= x2 - p2_x * cos1 - p2_y * sin1;
    y_n= y2 - p2_y * cos1 + p2_x * sin1;
    if (trn.add_point(x_n, y_n) ||
        trn.add_point(x2 - p2_x, y2 - p2_y) ||
        fill_gap(&trn, x2, y2, -p2_x, -p2_y, p1_x, p1_y, m_d, &empty_gap2))
      return 1;
    empty_gap1= false;
  }
  if ((!empty_gap2 && trn.add_point(x2 + p1_x, y2 + p1_y)) ||
      trn.add_point(x1 + p1_x, y1 + p1_y))
    return 1;

  if (round_p1 && fill_half_circle(&trn, x1, y1, p1_x, p1_y))
    return 1;

  if (trn.add_point(x1 - p1_x, y1 - p1_y) ||
      (!empty_gap1 && trn.add_point(x2 - p1_x, y2 - p1_y)))
    return 1;
  return trn.complete_simple_poly();
}


int Item_func_buffer::transporter::add_last_edge_buffer()
{
  gcalc_operation_transporter trn(m_fn, m_heap);
  double e1_x, e1_y, p1_x, p1_y;

  ++m_nshapes;
  if (trn.start_simple_poly())
    return 1;

  calculate_perpendicular(x1, y1, x2, y2, m_d, &e1_x, &e1_y, &p1_x, &p1_y);

  if (trn.add_point(x1 + p1_x, y1 + p1_y) ||
      trn.add_point(x1 - p1_x, y1 - p1_y) ||
      trn.add_point(x2 - p1_x, y2 - p1_y) ||
      fill_half_circle(&trn, x2, y2, -p1_x, -p1_y) ||
      trn.add_point(x2 + p1_x, y2 + p1_y))
    return 1;
  return trn.complete_simple_poly();
}


int Item_func_buffer::transporter::add_point_buffer(double x, double y)
{
  gcalc_operation_transporter trn(m_fn, m_heap);

  m_nshapes++;
  if (trn.start_simple_poly())
    return 1;
  if (trn.add_point(x - m_d, y) ||
      fill_half_circle(&trn, x, y, -m_d, 0.0) ||
      trn.add_point(x + m_d, y) ||
      fill_half_circle(&trn, x, y, m_d, 0.0))
    return 1;
  return trn.complete_simple_poly();
}


int Item_func_buffer::transporter::start_line()
{
  m_npoints= 0;
  int_start_line();
  return 0;
}


int Item_func_buffer::transporter::start_poly()
{
  ++m_nshapes;
  return gcalc_operation_transporter::start_poly();
}


int Item_func_buffer::transporter::start_ring()
{
  m_npoints= 0;
  return gcalc_operation_transporter::start_ring();
}


int Item_func_buffer::transporter::add_point(double x, double y)
{
  if (m_npoints && x == x2 && y == y2)
    return 0;

  ++m_npoints;

  if (m_npoints == 1)
  {
    x00= x;
    y00= y;
  }
  else if (m_npoints == 2)
  {
    x01= x;
    y01= y;
  }
  else if (add_edge_buffer(x, y, (m_npoints == 3) && line_started(), false))
    return 1;

  x1= x2;
  y1= y2;
  x2= x;
  y2= y;

  return line_started() ? 0 : gcalc_operation_transporter::add_point(x, y);
}


int Item_func_buffer::transporter::complete()
{
  if (m_npoints)
  {
    if (m_npoints == 1)
    {
      if (add_point_buffer(x2, y2))
        return 1;
    }
    else if (m_npoints == 2)
    {
      if (add_edge_buffer(x1, y1, true, true))
        return 1;
    }
    else if (line_started())
    {
      if (add_last_edge_buffer())
        return 1;
    }
    else
    {
      if (x2 != x00 && y2 != y00)
      {
        if (add_edge_buffer(x00, y00, false, false))
          return 1;
        x1= x2;
        y1= y2;
        x2= x00;
        y2= y00;
      }
      if (add_edge_buffer(x01, y01, false, false))
        return 1;
    }
  }

  return 0;
}


int Item_func_buffer::transporter::complete_line()
{
  if (complete())
    return 1;
  int_complete_line();
  return 0;
}


int Item_func_buffer::transporter::complete_ring()
{
  return complete() ||
         gcalc_operation_transporter::complete_ring();
}


String *Item_func_buffer::val_str(String *str_value)
{
  DBUG_ENTER("Item_func_buffer::val_str");
  DBUG_ASSERT(fixed == 1);
  String *obj= args[0]->val_str(&tmp_value);
  double dist= args[1]->val_real();
  Geometry_buffer buffer;
  Geometry *g;
  uint32 union_pos;
  uint32 srid= 0;
  String *str_result= NULL;
  transporter trn(&func, &collector, dist);

  null_value= 1;
  if (args[0]->null_value || args[1]->null_value ||
      !(g= Geometry::construct(&buffer, obj->ptr(), obj->length())))
    goto mem_error;

  if (func.reserve_op_buffer(2))
    goto mem_error;
  /* will specify operands later */
  union_pos= func.get_next_operation_pos();
  func.add_operation((dist > 0.0) ? gcalc_function::op_union :
                                    gcalc_function::op_difference, 0);

  GCALC_DBUG_STARTFILE("/home/hf/linebuffer");
  if (g->store_shapes(&trn))
    goto mem_error;

  GCALC_DBUG_STOP;
  func.add_operands_to_op(union_pos, trn.m_nshapes);
  collector.prepare_operation();
  if (func.alloc_states())
    goto mem_error;
  operation.init(&func);

  if (operation.count_all(&collector) ||
      operation.get_result(&res_receiver))
    goto mem_error;


  str_value->set_charset(&my_charset_bin);
  if (str_value->reserve(SRID_SIZE, 512))
    goto mem_error;
  str_value->length(0);
  str_value->q_append(srid);

  if (!Geometry::create_from_opresult(&buffer, str_value, res_receiver))
    goto mem_error;

  null_value= 0;
  str_result= str_value;
mem_error:
  collector.reset();
  func.reset();
  scan_it.reset();
  res_receiver.reset();
  DBUG_RETURN(str_result);
}


longlong Item_func_isempty::val_int()
{
  DBUG_ASSERT(fixed == 1);
  String tmp;
  String *swkb= args[0]->val_str(&tmp);
  Geometry_buffer buffer;
  
  null_value= args[0]->null_value ||
              !(Geometry::construct(&buffer, swkb->ptr(), swkb->length()));
  return null_value ? 1 : 0;
}


/**
  @todo
    Ramil or Holyfoot, add real IsSimple calculation
*/
longlong Item_func_issimple::val_int()
{
  DBUG_ASSERT(fixed == 1);
  String tmp;
  String *swkb= args[0]->val_str(&tmp);
  Geometry_buffer buffer;
  
  null_value= args[0]->null_value ||
              !(Geometry::construct(&buffer, swkb->ptr(), swkb->length()));
  /* TODO: Ramil or Holyfoot, add real IsSimple calculation */
  return 0;
}


longlong Item_func_isclosed::val_int()
{
  DBUG_ASSERT(fixed == 1);
  String tmp;
  String *swkb= args[0]->val_str(&tmp);
  Geometry_buffer buffer;
  Geometry *geom;
  int isclosed= 0;				// In case of error

  null_value= (!swkb || 
	       args[0]->null_value ||
	       !(geom=
		 Geometry::construct(&buffer, swkb->ptr(), swkb->length())) ||
	       geom->is_closed(&isclosed));

  return (longlong) isclosed;
}

/*
  Numerical functions
*/


longlong Item_func_dimension::val_int()
{
  DBUG_ASSERT(fixed == 1);
  uint32 dim= 0;				// In case of error
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;
  const char *dummy;

  null_value= (!swkb || 
	       args[0]->null_value ||
	       !(geom= Geometry::construct(&buffer, swkb->ptr(), swkb->length())) ||
	       geom->dimension(&dim, &dummy));
  return (longlong) dim;
}


longlong Item_func_numinteriorring::val_int()
{
  DBUG_ASSERT(fixed == 1);
  uint32 num= 0;				// In case of error
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;

  null_value= (!swkb || 
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(), swkb->length())) ||
	       geom->num_interior_ring(&num));
  return (longlong) num;
}


longlong Item_func_numgeometries::val_int()
{
  DBUG_ASSERT(fixed == 1);
  uint32 num= 0;				// In case of errors
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;

  null_value= (!swkb ||
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(), swkb->length())) ||
	       geom->num_geometries(&num));
  return (longlong) num;
}


longlong Item_func_numpoints::val_int()
{
  DBUG_ASSERT(fixed == 1);
  uint32 num= 0;				// In case of errors
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;

  null_value= (!swkb ||
	       args[0]->null_value ||
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(), swkb->length())) ||
	       geom->num_points(&num));
  return (longlong) num;
}


double Item_func_x::val_real()
{
  DBUG_ASSERT(fixed == 1);
  double res= 0.0;				// In case of errors
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;

  null_value= (!swkb ||
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(), swkb->length())) ||
	       geom->get_x(&res));
  return res;
}


double Item_func_y::val_real()
{
  DBUG_ASSERT(fixed == 1);
  double res= 0;				// In case of errors
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;

  null_value= (!swkb ||
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(), swkb->length())) ||
	       geom->get_y(&res));
  return res;
}


double Item_func_area::val_real()
{
  DBUG_ASSERT(fixed == 1);
  double res= 0;				// In case of errors
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;
  const char *dummy;

  null_value= (!swkb ||
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(), swkb->length())) ||
	       geom->area(&res, &dummy));
  return res;
}

double Item_func_glength::val_real()
{
  DBUG_ASSERT(fixed == 1);
  double res= 0;				// In case of errors
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  Geometry *geom;

  null_value= (!swkb || 
	       !(geom= Geometry::construct(&buffer,
                                           swkb->ptr(),
                                           swkb->length())) ||
	       geom->geom_length(&res));
  return res;
}

longlong Item_func_srid::val_int()
{
  DBUG_ASSERT(fixed == 1);
  String *swkb= args[0]->val_str(&value);
  Geometry_buffer buffer;
  
  null_value= (!swkb || 
	       !Geometry::construct(&buffer,
                                    swkb->ptr(), swkb->length()));
  if (null_value)
    return 0;

  return (longlong) (uint4korr(swkb->ptr()));
}


double Item_func_distance::val_real()
{
  bool above_cur_point, cur_point_edge;
  const gcalc_scan_iterator::point *evpos;
  const gcalc_heap::info *cur_point, *dist_point;
  gcalc_scan_events ev;
  double t, distance, cur_distance;
  double x1, x2, y1, y2;
  double ex, ey, vx, vy, e_sqrlen;
  uint obj2_si;
  gcalc_operation_transporter trn(&func, &collector);

  DBUG_ENTER("Item_func_distance::val_real");
  DBUG_ASSERT(fixed == 1);
  String *res1= args[0]->val_str(&tmp_value1);
  String *res2= args[1]->val_str(&tmp_value2);
  Geometry_buffer buffer1, buffer2;
  Geometry *g1, *g2;


  if ((null_value= (args[0]->null_value || args[1]->null_value ||
          !(g1= Geometry::construct(&buffer1, res1->ptr(), res1->length())) ||
          !(g2= Geometry::construct(&buffer2, res2->ptr(), res2->length())))))
    goto mem_error;

  if ((g1->get_class_info()->m_type_id == Geometry::wkb_point) &&
      (g2->get_class_info()->m_type_id == Geometry::wkb_point))
  {
    if (((Gis_point *)g1)->get_xy(&x1, &y1) ||
        ((Gis_point *)g2)->get_xy(&x2, &y2))
      goto mem_error;
    ex= x2 - x1;
    ey= y2 - y1;
    DBUG_RETURN(sqrt(ex * ex + ey * ey));
  }

  if (func.reserve_op_buffer(1))
    goto mem_error;
  func.add_operation(gcalc_function::op_intersection, 2);

  if (g1->store_shapes(&trn))
    goto mem_error;
  obj2_si= func.get_nshapes();
  if (g2->store_shapes(&trn) || func.alloc_states())
    goto mem_error;

  collector.prepare_operation();
  scan_it.init(&collector);

  above_cur_point= false;
  distance= DBL_MAX;
  while (scan_it.more_points())
  {
    if (scan_it.step())
      goto mem_error;
    evpos= scan_it.get_event_position();
    ev= scan_it.get_event();
    cur_point= evpos->pi;

    /*
       handling intersection we only need to check if it's the intersecion
       of objects 1 and 2. In this case distance is 0
    */
    if (ev == scev_intersection)
    {
      if ((evpos->get_next()->pi->shape >= obj2_si) !=
            (cur_point->shape >= obj2_si))
      {
        distance= 0;
        goto exit;
      }
      continue;
    }

    /*
       if we get 'scev_point | scev_end | scev_two_ends' we don't need
       to check for intersection of objects.
       Though we need to calculate distances.
    */
    if (ev & (scev_point | scev_end | scev_two_ends))
      goto count_distance;

    /*
       having these events we need to check for possible intersection
       of objects
       scev_thread | scev_two_threads | scev_single_point
    */
    DBUG_ASSERT(ev & (scev_thread | scev_two_threads | scev_single_point));

    func.clear_state();
    for (gcalc_point_iterator pit(&scan_it); pit.point() != evpos; ++pit)
    {
      gcalc_shape_info si= pit.point()->get_shape();
      if ((func.get_shape_kind(si) == gcalc_function::shape_polygon))
        func.invert_state(si);
    }
    func.invert_state(evpos->get_shape());
    if (func.count())
    {
      /* Point of one object is inside the other - intersection found */
      distance= 0;
      goto exit;
    }


count_distance:
    if (cur_point->shape >= obj2_si)
      continue;
    cur_point_edge= !cur_point->is_bottom();

    for (dist_point= collector.get_first(); dist_point; dist_point= dist_point->get_next())
    {
      /* We only check vertices of object 2 */
      if (dist_point->shape < obj2_si)
        continue;

      /* if we have an edge to check */
      if (dist_point->left)
      {
        t= count_edge_t(dist_point, dist_point->left, cur_point,
                        ex, ey, vx, vy, e_sqrlen);
        if ((t>0.0) && (t<1.0))
        {
          cur_distance= distance_to_line(ex, ey, vx, vy, e_sqrlen);
          if (distance > cur_distance)
            distance= cur_distance;
        }
      }
      if (cur_point_edge)
      {
        t= count_edge_t(cur_point, cur_point->left, dist_point,
                        ex, ey, vx, vy, e_sqrlen);
        if ((t>0.0) && (t<1.0))
        {
          cur_distance= distance_to_line(ex, ey, vx, vy, e_sqrlen);
          if (distance > cur_distance)
            distance= cur_distance;
        }
      }
      cur_distance= distance_points(cur_point, dist_point);
      if (distance > cur_distance)
        distance= cur_distance;
    }
  }
exit:
  collector.reset();
  func.reset();
  scan_it.reset();
  DBUG_RETURN(distance);
mem_error:
  null_value= 1;
  DBUG_RETURN(0);
}


#endif /*HAVE_SPATIAL*/
