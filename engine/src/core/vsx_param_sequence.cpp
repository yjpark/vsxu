/**
* Project: VSXu Engine: Realtime modular visual programming engine.
*
* This file is part of Vovoid VSXu Engine.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Lesser General Public License (LGPL)
*
* VSXu Engine is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "vsx_engine.h"
#include "vsx_param_sequence.h"

vsx_bezier_calc bez_calc;

vsx_param_sequence_item::vsx_param_sequence_item()
{
  total_length = 1.0f;
  value = "";
  interpolation = 1;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void vsx_param_sequence::execute(float ptime, float blend)
{
  if (items.size() < 2)
  {
    if (items.size())
    {
      param->set_string(items[0].value);
    }
  }
  if
  (
    line_time == 0
    &&
    line_cur == 0
  )
  {
    if (cur_val == "")
    {
      cur_val = items[0].value;
      cur_delay = items[0].total_length;
      cur_interpolation = items[0].interpolation;
      if (items.size() > 1)
      {
        to_val = items[1].value;
      }
    }
  }

  int c = 0;

  line_time += ptime;
  //printf("ptime: %f  line_time: %f line_cur: %d\n",ptime,line_time,line_cur);

  if (ptime < 0)
  {
    if ((line_time) < 0 && line_cur != 0)
    {
      while (line_time < 0)
      {
        ++c;
        --line_cur;
        if (line_cur < 0)
        {
          line_cur = 0;
          line_time = 0;
        } else
        {
          line_time +=  items[line_cur].total_length;
          cur_val = items[line_cur].value;
          cur_delay = items[line_cur].total_length;
          cur_interpolation = items[line_cur].interpolation;
          to_val = items[line_cur+1].value;
        }
      }
    }
  }
  else
  {
    if (cur_delay != -1)
    {
      while (line_time > cur_delay && cur_delay != -1)
      {
        ++c;
        line_time -= items[line_cur].total_length;
        ++line_cur;
        cur_interpolation = items[line_cur].interpolation;
        cur_delay = items[line_cur].total_length;
        cur_val = to_val;
        if (line_cur >= (int)items.size()-1)
        {
          cur_delay = -1;
        }
        else
        {
          if (line_cur >= (int)items.size()-1)
          {
            to_val = items[line_cur].value;
          }
          else
          {
            to_val = items[line_cur+1].value;
          }
        }
      }
    }
  }
  //printf("line_cur: %d  cur_val: %s to_val: %s line_time: %f curdel: \n",line_cur, cur_val.c_str(),to_val.c_str(),line_time,cur_delay);
  if (to_val.size() && cur_val.size())
  {
    float t = (line_time/cur_delay);
    if (param->module_param->type == VSX_MODULE_PARAM_ID_FLOAT)
    {
      ++param->module->param_updates;
      ++((vsx_module_param_float*)param->module_param)->updates;
      float cv = s2f(cur_val);
      float ev = s2f(to_val);
      float dv = ev-cv;
      float result_value = cv;

      //printf("sequence execute: %s %f\n", cur_val.c_str(), cv);
      // 0 = no interpolation
      // 1 = linear interpolation
      // 2 = cosine interpolation
      // 3 = no interpolation + param_interpolator

      if (cur_interpolation == 1)
      {
        result_value = cv + dv * t;
        goto execute_float_value_set;
      }
      if (cur_interpolation == 2)
      {
        float f =
          (
            1
            -
            cos(
              t * PI_FLOAT
            )
          )
          * 0.5f
        ;
        result_value = cv * (1.0f-f) + ev * f;
        goto execute_float_value_set;
      }
      //if (cur_interpolation == 3)
      //{
      //  ((vsx_engine*)engine)->interpolation_list.set_target_value(param, f2s(cv), 0, interp_time);
      //}

      if (cur_interpolation == 4)
      {
        bez_calc.x0 = 0.0f;
        bez_calc.y0 = cv;
        bez_calc.x1 = items[line_cur].handle1.x;
        bez_calc.y1 = cv+items[line_cur].handle1.y;
        bez_calc.x2 = items[line_cur].handle2.x;
        bez_calc.y2 = ev+items[line_cur].handle2.y;
        bez_calc.x3 = 1.0f;
        bez_calc.y3 = ev;
        bez_calc.init();

        float tt = bez_calc.t_from_x(t);
        result_value = bez_calc.y_from_t(tt);
        goto execute_float_value_set;
      }

      execute_float_value_set:
      {
        if (blend < 1.0f)
        {
          float current_value = ((vsx_module_param_float*)param->module_param)->get_internal();
          result_value = (1.0f - blend) * current_value + blend * result_value;
        }
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(result_value);
      }
    } else
    if (param->module_param->type == VSX_MODULE_PARAM_ID_QUATERNION)
    {
      vsx_quaternion cv, ev;
      cv.from_string(cur_val);
      ev.from_string(to_val);

      // 0 = no interpolation
      // 1 = linear interpolation
      // 2 = cosine interpolation
      // 3 = no interpolation + param_interpolator
      if (cur_interpolation == 0)
      {
        cv.normalize();
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(cv.x,0);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(cv.y,1);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(cv.z,2);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(cv.w,3);
      } else
      if (cur_interpolation == 1)
      {
        cv.normalize();
        ev.normalize();
        vsx_quaternion iq;
        iq.slerp(cv, ev, t);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.x,0);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.y,1);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.z,2);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.w,3);
      }
      else
      if (cur_interpolation == 2)
      {
        cv.normalize();
        ev.normalize();
        vsx_quaternion iq;
        iq.cos_slerp(cv, ev, t);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.x,0);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.y,1);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.z,2);
        ((vsx_module_param_quaternion*)param->module_param)->set_internal(iq.w,3);
      }
    }
    else
    if (param->module_param->type == VSX_MODULE_PARAM_ID_FLOAT3)
    {
    }
  }
}

float vsx_param_sequence::calculate_total_time(bool no_cache)
{
	if (no_cache)
  {
    total_time = 0.0f;
  }
	else
  {
    if (total_time != 0.0f)
    {
      return total_time;
    }
  }

  float last_length;
  for (std::vector<vsx_param_sequence_item>::iterator it = items.begin(); it != items.end(); ++it)
  {
    total_time += (*it).total_length;
    last_length = (*it).total_length;
  }
  total_time -= last_length;
  return total_time;
}

vsx_string vsx_param_sequence::dump()
{
  vsx_string res = "";
  std::list<vsx_string> ml;
  for (std::vector<vsx_param_sequence_item>::iterator it = items.begin(); it != items.end(); ++it)
  {
    //printf("adding dumpstring: %s\n",(f2s((*it).delay)+";"+f2s((*it).interpolation)+";"+base64_encode((*it).value)).c_str());
    ml.push_back(f2s((*it).total_length)+";"+i2s((*it).interpolation)+";"+base64_encode((*it).get_value()));
  }
  vsx_string deli = "|";
  res = implode(ml, deli);
  return res;
}

void vsx_param_sequence::inject(vsx_string ij)
{
	total_time = 0.0f; // reset total time for re-calculation
  items.clear();
  vsx_string deli = "|";
  std::list<vsx_string> pl;
  explode(ij, deli, pl);
  for (std::list<vsx_string>::iterator it = pl.begin(); it != pl.end(); ++it) {
    std::vector<vsx_string> pld;
    vsx_string pdeli = ";";
    explode((*it),pdeli,pld);
    vsx_param_sequence_item pa;
    pa.total_length = s2f(pld[0]);
    pa.interpolation = s2i(pld[1]);
    if (pa.interpolation < 4) {
      pa.value = base64_decode(pld[2]);
    } else
    if (pa.interpolation == 4) {
      std::vector<vsx_string> pld_l;
      vsx_string pdeli_l = ":";
      vsx_string vtemp = base64_decode(pld[2]);
      //printf("value: %s\n",vtemp.c_str());
      explode(vtemp,pdeli_l,pld_l);
      pa.value = pld_l[0];
      pa.handle1.from_string(pld_l[1]);
      pa.handle2.from_string(pld_l[2]);
    }
    items.push_back(pa);
    //printf("inject delay: %s\n",pld[0].c_str());
  }
}

vsx_param_sequence::vsx_param_sequence(int p_type,vsx_engine_param* param)
{
  interp_time = 10;
  cur_val = to_val = "";
  last_time = 0.0f;
  line_time = 0.0f;
  line_cur = 0;
  p_time = 0;
  cur_delay = 0.0f;
  vsx_param_sequence_item pa;
  pa.total_length = 3;

  switch (p_type)
  {
    case VSX_MODULE_PARAM_ID_FLOAT:
    {
      pa.interpolation = 1;
      pa.value =  f2s(((vsx_module_param_float*)param->module_param)->get());
      items.push_back(pa);
      items.push_back(pa);
    }
    break;
    case VSX_MODULE_PARAM_ID_QUATERNION:
    {
      pa.interpolation = 0;
      pa.value = param->get_string();
      items.push_back(pa);
      items.push_back(pa);
    }
    break;
  }
}

void vsx_param_sequence::rescale_time(float start, float scale)
{
  // reset total time for re-calculation
  total_time = 0.0f;
  float accum_time = 0.0f;
  bool first = true;
  for (unsigned long i = 0; i < items.size(); ++i)
  {
    accum_time += items[i].total_length;
    if (accum_time > start)
    {
      // we found our target position yay
      if (first)
      {
        // cut in half
        float second_half = accum_time-start;
        float first_half = items[i].total_length - second_half;
        items[i].total_length = first_half + second_half*scale;
        first = false;
      }
      else
      {
        items[i].total_length *= scale;
      }
    }
  }
}

vsx_param_sequence::vsx_param_sequence()
{
  interp_time = 10;
  cur_val = to_val = "";
  last_time = 0.0f;
  line_time = 0.0f;
  line_cur = 0;
  p_time = 0;
	total_time = 0.0f;
  cur_delay = 0.0f;
}

void vsx_param_sequence::update_line(vsx_command_list* dest, vsx_command_s* cmd_in, vsx_string cmd_prefix)
{
  VSX_UNUSED(dest);
  VSX_UNUSED(cmd_prefix);
	total_time = 0.0f; // reset total time for re-calculation
#ifdef VSXU_DEBUG
  printf("UPDATE_LINE in engine %s\n",cmd_in->raw.c_str());
#endif
  vsx_param_sequence_item pa;
  pa.total_length = s2f(cmd_in->parts[5]);
  pa.interpolation = s2i(cmd_in->parts[6]);
  if (pa.interpolation < 4)
  {
    pa.value = base64_decode(cmd_in->parts[4]);
  	//printf("value in string format: %s\n", pa.value.c_str());
  }
  else
  if (pa.interpolation == 4) {
    std::vector<vsx_string> pld_l;
    vsx_string pdeli_l = ":";
    vsx_string vtemp = base64_decode(cmd_in->parts[4]);
    //printf("value: %s\n",vtemp.c_str());
    explode(vtemp,pdeli_l,pld_l);
    pa.value = pld_l[0];
    pa.handle1.from_string(pld_l[1]);
    pa.handle2.from_string(pld_l[2]);
  }

  items[s2i(cmd_in->parts[7])] = pa;
  //dest->add_raw(cmd_prefix+"pseq_r_ok update "+cmd_in->parts[2]+" "+cmd_in->parts[3]+" "+cmd_in->parts[4]+" "+cmd_in->parts[5]+" "+cmd_in->parts[6]+" "+cmd_in->parts[7]);
  cur_val = to_val = "";
  last_time = 0.0;
  line_time = 0.0;
  line_cur = 0;
  p_time = 0;
  //printf("pseql_r a %s\n",cmd_in->parts[4].c_str());
}

void vsx_param_sequence::insert_line(vsx_command_list* dest, vsx_command_s* cmd_in, vsx_string cmd_prefix)
{
	total_time = 0.0f; // reset total time for re-calculation
  //printf("INSERT_LINE in engine %s\n",cmd_in->raw.c_str());
  long after_pos = s2i(cmd_in->parts[7]);
  float delay = s2f(cmd_in->parts[5]);
  //printf("delay: %f\n",delay);
  std::vector<vsx_param_sequence_item>::iterator it = items.begin();
  if (after_pos == (long)items.size()-1) {
    printf("last position, interpolation type: %s\n",cmd_in->parts[6].c_str());
    items[items.size()-1].total_length = delay;
    vsx_param_sequence_item pa;
    pa.value = base64_decode(cmd_in->parts[4]);
    pa.total_length = 1;
    pa.interpolation = s2i(cmd_in->parts[6]);
    items.push_back(pa);
  } else {
    int i;
    for (i = 0; i < after_pos; ++i) {++it;
    //  printf("incrementing it\n");
    }

    //printf("i : %d\n",i);

    vsx_param_sequence_item pa;
    pa.total_length = (*it).total_length-delay;
    (*it).total_length = delay;
    pa.interpolation = s2i(cmd_in->parts[6]);
    if (pa.interpolation < 4) {
      pa.value = base64_decode(cmd_in->parts[4]);
    } else
    if (pa.interpolation == 4) {
      std::vector<vsx_string> pld_l;
      vsx_string pdeli_l = ":";
      vsx_string vtemp = base64_decode(cmd_in->parts[4]);
      //printf("value: %s\n",vtemp.c_str());
      explode(vtemp,pdeli_l,pld_l);
      pa.value = pld_l[0];
      pa.handle1.from_string(pld_l[1]);
      pa.handle2.from_string(pld_l[2]);
    }

    ++it;
    items.insert(it, pa);
  }
  cur_val = to_val = "";
  last_time = 0.0;
  line_time = 0.0;
  line_cur = 0;
  p_time = 0;
  dest->add_raw(cmd_prefix+"pseq_r_ok insert "+cmd_in->parts[2]+" "+param->name+" "+cmd_in->parts[4]+" "+cmd_in->parts[5]+" "+cmd_in->parts[6]+" "+cmd_in->parts[7]);
  //printf("pseql_r insert %s\n",base64_decode(cmd_in->parts[4]).c_str());
}

void vsx_param_sequence::remove_line(vsx_command_list* dest, vsx_command_s* cmd_in, vsx_string cmd_prefix)
{
  /*
  float last_time; // last time we were called, to see if we should trace back
  float line_time; // current line time (accumulated)
  int line_cur; // current line
  vsx_string cur_val, to_val;
  float cur_delay;
  int cur_interpolation;
  float total_time;
  bool repeat; // repeat this or stop at end?
  float time_scaler; // global multiplier
  float p_time; // internal time
  float interp_time; // interpolation time for use when feeding the engine interpolator
  */
  last_time = 0.0f;
  line_time = 0.0f;
  line_cur = 0;
  cur_val = "";
  to_val = "";
  cur_delay = 0.0f;
  cur_interpolation = 1;
  total_time = 0.0f; // reset total time for re-calculation
  repeat = false;
  time_scaler = 1.0f;
  p_time = 0.0f;
  interp_time = 0.0f;

  //printf("REMOVE_LINE in engine %s\n",cmd_in->raw.c_str());
  long pos = s2i(cmd_in->parts[4]);
  std::vector<vsx_param_sequence_item>::iterator it = items.begin();
  if (pos != 0)
  {
    for (int i = 0; i < pos; ++i) ++it;
    if (pos < (long)items.size()-1)
    {
      items[pos-1].total_length += items[pos].total_length;
    }
    items.erase(it);
  }
  p_time = 0;

  dest->add_raw(cmd_prefix+"pseq_r_ok remove "+cmd_in->parts[2]+" "+cmd_in->parts[3]+" "+cmd_in->parts[4]);
}
