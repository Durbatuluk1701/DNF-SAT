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

enum class FormulaType
{
  FVar,
  FNeg,
  FDisj,
  FConj
};

struct Formula
{
  FormulaType type;
  union
  {
    int var;
    struct Formula *neg;
    std::deque<Formula *> *vec;
  };
};

Formula *new_FVar(int var)
{
  Formula *f = new Formula;
  f->type = FormulaType::FVar;
  f->var = var;
  return f;
}

Formula *new_FNeg(Formula *fr)
{
  Formula *f = new Formula;
  f->type = FormulaType::FNeg;
  f->neg = fr;
  return f;
}

Formula *new_FDisj(std::deque<Formula *> *fvec)
{
  Formula *f = new Formula;
  f->type = FormulaType::FDisj;
  f->vec = fvec;
  return f;
}

Formula *new_FConj(std::deque<Formula *> *fvec)
{
  Formula *f = new Formula;
  f->type = FormulaType::FConj;
  f->vec = fvec;
  return f;
}

void print_formula(Formula *f, bool top)
{
  switch (f->type)
  {
  case FormulaType::FVar:
  {
    printf("%i", f->var);
    break;
  }
  case FormulaType::FNeg:
  {
    printf("~");
    print_formula(f->neg, false);
    break;
  }
  case FormulaType::FDisj:
  {
    auto size = f->vec->size();
    printf("(");
    for (int i = 0; i < size; ++i)
    {
      auto rec = (f->vec)->at(i);
      print_formula(rec, false);
      if (i < size - 1)
      {
        printf(" \\/ ");
      }
    }
    printf(")");
    break;
  }
  case FormulaType::FConj:
  {
    auto size = f->vec->size();
    printf("(");
    for (int i = 0; i < size; ++i)
    {
      auto rec = (f->vec)->at(i);
      print_formula(rec, false);
      if (i < size - 1)
      {
        printf(" /\\ ");
      }
    }
    printf(")");
    break;
  }
  default:
    break;
  }
  if (top)
  {
    printf("\n");
  }
}

void delete_Formula(Formula *f)
{
  if (f == nullptr)
    return;
  if (f->type == FormulaType::FNeg)
  {
    delete_Formula(f->neg);
  }
  else if (f->type == FormulaType::FDisj || f->type == FormulaType::FConj)
  {
    for (auto &sub_formula : *(f->vec))
    {
      delete_Formula(sub_formula);
    }
    delete f->vec;
  }
  delete f;
}

void flatten(Formula *f)
{
  switch (f->type)
  {
  case FormulaType::FVar:
  {
    // Do nothing, cannot flatten a var
    break;
  }
  case FormulaType::FNeg:
  {
    switch (f->neg->type)
    {
    case FormulaType::FVar:
    {
      // Do thing, cannot flatten ~ var
      break;
    }
    case FormulaType::FNeg:
    {
      auto fref = f->neg;
      f = f->neg->neg;
      delete fref;
      break;
    }
    case FormulaType::FConj:
    case FormulaType::FDisj:
    {
      throw std::runtime_error("Should not have conj or disj below neg");
    }
    }
    break;
  }
  case FormulaType::FDisj:
  {
    auto size = f->vec->size();
    for (int i = 0; i < size; ++i)
    {
      auto fv = f->vec->at(i);
      switch (fv->type)
      {
      case FormulaType::FVar:
      case FormulaType::FNeg:
      case FormulaType::FConj:
      {
        // Do nothing, cannot flatten
        break;
      }
      case FormulaType::FDisj:
      {
        // Take out the below vectors stuff
        f->vec->insert(f->vec->end(), fv->vec->begin(), fv->vec->end());
        delete fv;
        f->vec->erase(f->vec->begin() + i);
        --size;
        --i;
        break;
      }
      }
    }
    break;
  }
  case FormulaType::FConj:
  {
    auto size = f->vec->size();
    for (int i = 0; i < size; ++i)
    {
      auto fv = f->vec->at(i);
      flatten(fv);
      switch (fv->type)
      {
      case FormulaType::FVar:
      case FormulaType::FNeg:
      {
        // Do nothing, cannot flatten
        break;
      }
      case FormulaType::FDisj:
      {
        throw std::runtime_error("Should not have Disj below conj");
      }
      case FormulaType::FConj:
      {
        // Take out the below vectors stuff
        f->vec->insert(f->vec->end(), fv->vec->begin(), fv->vec->end());
        delete fv;
        f->vec->erase(f->vec->begin() + i);
        --size;
        --i;
        break;
      }
      }
    }
    break;
  }
  }
}

Formula *formula_cross(std::deque<Formula *> *fvec)
{
  std::deque<Formula *> *start_queue = new std::deque<Formula *>;
  std::deque<std::deque<Formula *> *> ret_vec = {start_queue};
  for (auto f : *fvec)
  {
    switch (f->type)
    {
    case FormulaType::FVar:
    case FormulaType::FNeg:
    {
      for (auto &vec : ret_vec)
      {
        vec->push_back(f);
      }
      break;
    }
    case FormulaType::FDisj:
    {
      auto size = ret_vec.size();
      while (size > 0)
      {
        auto vec = ret_vec.front();
        ret_vec.pop_front();
        --size;
        for (auto form : *(f->vec))
        {
          std::deque<Formula *> *cur_vec = new std::deque<Formula *>(*vec);
          cur_vec->push_back(form);
          ret_vec.push_back(cur_vec);
        }
      }
      break;
    }
    case FormulaType::FConj:
    {
      for (auto vec : ret_vec)
      {
        vec->insert(vec->end(), f->vec->begin(), f->vec->end());
      }
      break;
    }
    }
  }
  std::deque<Formula *> *final_vec = new std::deque<Formula *>;
  for (auto &vec : ret_vec)
  {
    final_vec->push_back(new_FConj(vec));
  }
  return new_FDisj(final_vec);
}

void to_dnf(Formula **f)
{
  switch ((*f)->type)
  {
  case FormulaType::FVar:
  {
    // Do nothing, it is itself
    break;
  }
  case FormulaType::FDisj:
  {
    for (auto &ele : *((*f)->vec))
    {
      to_dnf(&ele);
    }
    auto new_f = new_FDisj((*f)->vec);
    *f = new_f;
    flatten(*f);
    break;
  }
  case FormulaType::FNeg:
  {
    Formula *fr = (*f)->neg;
    switch (fr->type)
    {
    case FormulaType::FVar:
    {
      // Do nothing, ~ var = ~ var
      break;
    }
    case FormulaType::FNeg:
    {
      // Delete the old structs
      // Replace with the lower level call
      Formula *double_neg = (fr)->neg;
      to_dnf(&double_neg);
      *f = double_neg;
    }
    case FormulaType::FConj:
    {
      std::deque<Formula *> *ret_vec = new std::deque<Formula *>;
      for (auto ele : *(fr->vec))
      {
        Formula *new_form = new_FNeg(ele);
        to_dnf(&new_form);
        ret_vec->push_back(new_form);
      }
      Formula *ret_form = new_FConj(ret_vec);
      *f = ret_form;
      flatten(ret_form);
      break;
    }
    case FormulaType::FDisj:
    {
      std::deque<Formula *> *ret_vec = new std::deque<Formula *>;
      for (auto ele : *(fr->vec))
      {
        Formula *new_form = new_FNeg(ele);
        to_dnf(&new_form);
        ret_vec->push_back(new_form);
      }
      Formula *ret_form = new_FDisj(ret_vec);
      *f = ret_form;
      flatten(ret_form);
      break;
    }
    }
    break;
  }
  case FormulaType::FConj:
  {
    for (auto &ele : *((*f)->vec))
    {
      to_dnf(&ele);
    }
    *f = formula_cross((*f)->vec);
    // print_formula(*f, true);
    flatten(*f);
    // print_formula(*f, true);
    break;
  }
  }
}

Formula *proc_line(std::string line)
{
  std::istringstream ss(line);
  std::string token;
  std::deque<Formula *> *ret_val = new std::deque<Formula *>;
  while (ss >> token)
  {
    int val = std::stoi(token);
    if (val == 0)
    {
      break;
    }
    if (val > 0)
    {
      ret_val->push_back(new_FVar(val));
    }
    else
    {
      ret_val->push_back(new_FNeg(new_FVar(std::abs(val))));
    }
  }
  return new_FDisj(ret_val);
}

bool disj_below_neg(Formula *f, bool in_neg)
{
  switch (f->type)
  {
  case FormulaType::FVar:
    return false;
  case FormulaType::FNeg:
    return disj_below_neg(f->neg, true);
  case FormulaType::FDisj:
  {
    if (in_neg)
    {
      return true;
    }
    for (auto val : *(f->vec))
    {
      if (disj_below_neg(val, in_neg))
      {
        return true;
      }
    }
    return false;
  }
  case FormulaType::FConj:
  {
    for (auto val : *(f->vec))
    {
      if (disj_below_neg(val, in_neg))
      {
        return true;
      }
    }
    return false;
  }
  default:
  {
    return false;
  }
  }
}

bool disj_below_conj(Formula *f, bool in_conj)
{
  switch (f->type)
  {
  case FormulaType::FVar:
    return false;
  case FormulaType::FNeg:
    return disj_below_neg(f->neg, in_conj);
  case FormulaType::FDisj:
  {
    if (in_conj)
    {
      return true;
    }
    for (auto val : *(f->vec))
    {
      if (disj_below_neg(val, in_conj))
      {
        return true;
      }
    }
    return false;
  }
  case FormulaType::FConj:
  {
    for (auto val : *(f->vec))
    {
      if (disj_below_neg(val, true))
      {
        return true;
      }
    }
    return false;
  }
  default:
  {
    return false;
  }
  }
}

bool dnf_spec(Formula *f, bool in_neg, bool in_conj, bool in_disj)
{
  switch (f->type)
  {
  case FormulaType::FVar:
    return true;
  case FormulaType::FNeg:
    return dnf_spec(f->neg, true, in_conj, in_disj);
  case FormulaType::FDisj:
  {
    if (in_neg || in_conj || in_disj)
    {
      return false;
    }
    for (auto vec : *(f->vec))
    {
      if (!dnf_spec(vec, in_neg, in_conj, true))
      {
        return false;
      }
    }
    return true;
  }
  case FormulaType::FConj:
  {
    if (in_neg || in_conj)
    {
      return false;
    }
    for (auto vec : *(f->vec))
    {
      if (!dnf_spec(vec, in_neg, true, in_disj))
      {
        return false;
      }
    }
    return true;
  }
  default:
  {
    return false;
  }
  }
}

bool safe_insert_vec(std::deque<int64_t> &v, int64_t val)
{
  if (std::find(v.begin(), v.end(), -val) != v.end())
  {
    return false;
  }
  v.push_back(val);
  return true;
}

bool sat_conj(Formula *f)
{
  std::deque<int64_t> count_vec;
  switch (f->type)
  {
  case FormulaType::FVar:
    throw std::runtime_error("Var inside conj");
  case FormulaType::FNeg:
    throw std::runtime_error("Neg inside conj");
  case FormulaType::FDisj:
    throw std::runtime_error("Non-flat DISJ");
  case FormulaType::FConj:
    for (auto v : *(f->vec))
    {
      switch (v->type)
      {
      case FormulaType::FVar:
      {
        int64_t xi = v->var;
        if (!safe_insert_vec(count_vec, xi))
        {
          return false;
        }
        break;
      }
      case FormulaType::FNeg:
      {
        int64_t xi = v->neg->var;
        int64_t x_neg = -xi;
        if (!safe_insert_vec(count_vec, x_neg))
        {
          return false;
        }
        break;
      }
      case FormulaType::FDisj:
        throw std::runtime_error("Disj in Neg in Conj");
      case FormulaType::FConj:
        throw std::runtime_error("Conj in Neg in Conj");
      }
    }
    return true;
  default:
  {
    return false;
  }
  }
}

bool sat(Formula *f)
{
  std::atomic<bool> solution_found(false);
  switch (f->type)
  {
  case FormulaType::FVar:
    throw std::runtime_error("Top level var");
  case FormulaType::FNeg:
    throw std::runtime_error("Top level neg");
  case FormulaType::FDisj:
  {
    for (auto vec : *(f->vec))
    {
      if (solution_found.load())
      {
        return true;
      }
      else
      {
        bool res = sat_conj(vec);
        if (res)
        {
          solution_found.store(true);
        }
        if (solution_found.load())
        {
          return true;
        }
      }
    }
    return false;
  }
  case FormulaType::FConj:
    throw std::runtime_error("Why top level conj!");
  default:
  {
    return false;
  }
  }
}

// #define BIT_STR_SIZE ((num_vars / 4) + 1)

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
      // For each vector in acc_bit_strs, and for each vector in bit_strs do custom_AND_bool
      auto cur_acc_size = acc_bit_strs.size();
      for (uint i = 0; i < cur_acc_size; i++)
      {
        for (uint j = 0; j < bit_strs.size(); j++)
        {
          auto new_bit_str = custom_AND_bool(acc_bit_strs[i], bit_strs[j], num_vars);
          if (new_bit_str == nullptr)
          {
            // std::cout << "UNSAT" << std::endl;
            continue;
          }
          acc_bit_strs.push_back(new_bit_str);
        }
      }
      acc_bit_strs.erase(acc_bit_strs.begin(), acc_bit_strs.begin() + cur_acc_size);
    }
  }
  std::cout << "SAT: " << (acc_bit_strs.size() > 0 ? "true" : "false") << std::endl;

  return 0;
}
