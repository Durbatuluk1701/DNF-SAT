#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <deque>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <string.h>

std::deque<char *> process_line(std::string line, int num_vars)
{
  std::deque<char *> ret_val;
  std::istringstream ss(line);
  std::string token;
  while (ss >> token)
  {
    char *bit_str = static_cast<char *>(malloc((num_vars / 4) + 1));
    int val = std::stoi(token);
    if (val == 0)
    {
      break;
    }
    if (val > 0)
    {
      *bit_str &= ((0b11) << (2 * (val - 1)));
    }
    else
    {
      *bit_str &= ((0b10) << (2 * (val - 1)));
    }
    ret_val.push_back(bit_str);
  }
  return ret_val;
}

char *custom_AND_bool(char *form1, char *form2, int num_vars)
{
  // Create a return char* that is a deep copy of form1
  char *ret_val = static_cast<char *>(malloc((num_vars / 4) + 1));
  strcpy(ret_val, form1);
  for (int i = 0; i < (num_vars / 4) + 1; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      // Checking if either of the bits are 0
      if ((~form1[i] & (0b10 << (2 * j))) || (~form2[i] & (0b10 << (2 * j))))
      {
        // If they are, do the OR operation to assign the value
        ret_val[i] = form1[i] | (form2[i] & (0b11 << (2 * j)));
        continue;
      }
      // Otherwise, they must both be 1 (so they have been assigned)
      if ((form1[i] & (0b01 << (2 * j))) == (form2[i] & (0b01 << (2 * j))))
      {
        // If they have been assigned the same value, do nothing
        continue;
      }
      // Otherwise, they are incompatible assignments
      return nullptr;
    }
  }
  return ret_val;
}

bool mini_sat(char *formula, int num_vars)
{

  return false;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
    return -1;
  }

  std::string file_name = argv[1];

  std::ifstream file(file_name);
  if (!file.is_open())
  {
    std::cerr << "Failed to open file!" << std::endl;
    return -1;
  }

  std::string line;
  // std::deque<Formula *> *ret_form_vec = new std::deque<Formula *>;
  int num_vars = 0;
  int num_lines = 0;
  std::getline(file, line);
  std::istringstream ss(line);
  std::string token;
  for (int i = 0; i < 2; i++)
  {
    ss >> token;
  }
  ss >> token;
  num_vars = std::stoi(token);
  ss >> token;
  num_lines = std::stoi(token);
  std::deque<char *> acc_bit_strs;
  for (int i = 0; i < num_lines; i++)
  {
    std::getline(file, line);
    std::deque<char *> bit_strs = process_line(line, num_vars);
    if (acc_bit_strs.empty())
    {
      acc_bit_strs = bit_strs;
    }
    else
    {
      for (auto &bit_str : bit_strs)
      {
        acc_bit_strs.push_back(bit_str);
      }
    }
  }
  std::cout << "SAT: " << (acc_bit_strs.size() > 0 ? "true" : "false") << std::endl;

  return 0;
}
