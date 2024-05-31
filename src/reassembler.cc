#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.

  //crop substring into sliding window
  uint64_t first_unassembled_index = writer().bytes_pushed();
  uint64_t first_unacceptable_index = first_unassembled_index + writer().available_capacity();
  if (first_index >= first_unacceptable_index)
  {
    return;
  }
  if (first_index < first_unassembled_index)
  {
    if (first_index + data.length() <= first_unassembled_index)
    {
      return;
    }
    else{
      data = data.substr(first_unassembled_index - first_index);
      first_index = first_unassembled_index;
    }
  }

  
  uint64_t L = min(data.length(), first_unacceptable_index -first_index);
  if (L == data.length())
  {
    if (is_last_substring)
    {
      meet_last_substring = true;
    }
  }
  
  data = data.substr(0, L);

  //insert node
  auto iter = window.find(first_index);
  if (iter == window.end())
  {
    window.insert(make_pair(first_index, data));
    byte_internal += data.length();
  }else{
    if (window[first_index].length() < data.length())
    {
      byte_internal += data.length() - window[first_index].length();
      window[first_index] = data;
    }
  }

  // find the first node need to be merged
  iter = window.find(first_index);
  if(iter == window.end()){
    throw("error");
  }
  if (iter!=window.begin())
  {
    auto pre = prev(iter);
    if (pre->first + pre->second.length() >= iter->first)
    {
      iter = pre;
    }
  }

  //keep merge node until no overlap
  while (true)
  {
    auto next_iter = next(iter);
    if (next_iter == window.end())
    {
      break;
    }
    if (iter->first + iter->second.length() >= next_iter->first)
    {
      byte_internal -= next_iter->second.length() + iter->second.length();
      iter->second.replace(next_iter->first - iter->first, next_iter->second.length(), next_iter->second);
      byte_internal += iter->second.length();
      window.erase(next_iter);
    }else{
      break;
    }
  }
  //push the available string to buffer
  auto available_iter = window.find(first_unassembled_index);
  if (available_iter != window.end())
  {
    if(available_iter != window.begin()){
      throw("error");
    }
    output_.writer().push(available_iter->second);
    byte_internal -= available_iter->second.length();
    window.erase(available_iter);
  }

  if (meet_last_substring && byte_internal == 0)
  {
    output_.writer().close();
  }
  
  return;
  
  
  // uint64_t insert_index = first_index - first_unassembled_index;
  // if (insert_index >= buffer.length())
  // {
  //   uint64_t append_str_length = insert_index - buffer.length();
  //   string append_str(append_str_length, '0');
  //   string mask_str(data.length(),'1');
  //   buffer += append_str;
  //   buffer += data;
  //   mask += append_str;
  //   mask += mask_str;
  // }else{
  //   string mask_str(data.length(),'1');
  //   buffer.replace(insert_index, data.length(), data);
  //   mask.replace(insert_index, data.length(), mask_str);
  // }
  // uint64_t i = 0;
  // for (i = 0; i < buffer.size(); i++)
  // {
  //   if (mask[i]=='0')
  //   {
  //     break;
  //   } 
  // }

  // output_.writer().push(buffer.substr(0, i));
  // buffer.erase(0,i);
  // mask.erase(0,i);
  // byte_internal = 0;
  // for (uint64_t j = 0; j < buffer.size(); j++)
  // {
  //   if (mask[j]=='1')
  //   {
  //     byte_internal++;
  //   } 
  // }

  // if (meet_last_substring && byte_internal == 0)
  // {
  //   output_.writer().close();
  // }
  
  return;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return byte_internal;
}
