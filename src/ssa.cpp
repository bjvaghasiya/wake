/*
 * Copyright 2019 SiFive, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You should have received a copy of LICENSE.Apache2 along with
 * this software. If not, you may obtain a copy at
 *
 *    https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ssa.h"
#include "value.h"

const size_t Term::invalid;

Term::~Term() { }

void Redux::update(const SourceMap &map) {
  for (auto &x : args) x = map[x];
}

void Redux::format_args(std::ostream &os, TermFormat &format) const {
  bool first = true;
  for (auto x : args) {
    if (!first) os << " ";
    os << x;
    if (x >= format.id) os << " !!!";
    first = false;
  }
}

void RArg::format(std::ostream &os, TermFormat &format) const {
  os << "<arg>\n";
}

std::unique_ptr<Term> RArg::clone() const {
  return std::unique_ptr<Term>(new RArg(*this));
}

void RLit::format(std::ostream &os, TermFormat &format) const {
  HeapObject::format(os, value->get());
  os << "\n";
}

std::unique_ptr<Term> RLit::clone() const {
  return std::unique_ptr<Term>(new RLit(*this));
}

void RApp::format(std::ostream &os, TermFormat &format) const {
  os << "App(";
  format_args(os, format);
  os << ")\n";
}

std::unique_ptr<Term> RApp::clone() const {
  return std::unique_ptr<Term>(new RApp(*this));
}

void RPrim::format(std::ostream &os, TermFormat &format) const {
  os << name << "(";
  format_args(os, format);
  os << ")\n";
}

std::unique_ptr<Term> RPrim::clone() const {
  return std::unique_ptr<Term>(new RPrim(*this));
}

void RGet::format(std::ostream &os, TermFormat &format) const {
  os << "Get:" << index << "(";
  format_args(os, format);
  os << ")\n";
}

std::unique_ptr<Term> RGet::clone() const {
  return std::unique_ptr<Term>(new RGet(*this));
}

void RDes::format(std::ostream &os, TermFormat &format) const {
  os << "Des(";
  format_args(os, format);
  os << ")\n";
}

std::unique_ptr<Term> RDes::clone() const {
  return std::unique_ptr<Term>(new RDes(*this));
}

void RCon::format(std::ostream &os, TermFormat &format) const {
  os << "Con:" << kind << "(";
  format_args(os, format);
  os << ")\n";
}

std::unique_ptr<Term> RCon::clone() const {
  return std::unique_ptr<Term>(new RCon(*this));
}

static std::string pad(int depth) {
  return std::string(depth, ' ');
}

RFun::RFun(const RFun &o) : Term(o), output(o.output) {
  terms.reserve(o.terms.size());
  for (auto &x : o.terms)
    terms.emplace_back(x->clone());
}

void RFun::update(const SourceMap &map) {
  output = map[output];
}

void RFun::format(std::ostream &os, TermFormat &format) const {
  os << "FunRet:" << output;
  if (output > format.id + terms.size()) os << " !!!";
  os << "\n";
  format.depth += 2;
  for (auto &x : terms) {
    os << pad(format.depth+2) << ++format.id;
    if (!x->label.empty()) os << " (" << x->label << ")";
    os << " [" << x->meta << "] = ";
    x->format(os, format);
  }
  format.id -= terms.size();
  format.depth -= 2;
}

std::unique_ptr<Term> RFun::clone() const {
  return std::unique_ptr<Term>(new RFun(*this));
}

std::vector<std::unique_ptr<Term> > TargetScope::unwind(size_t newend) {
  std::vector<std::unique_ptr<Term> > out;
  out.reserve(terms.size() - newend);
  for (size_t i = newend; i < terms.size(); ++i)
    out.emplace_back(std::move(terms[i]));
  terms.resize(newend);
  return out;
}

void ScopeAnalysis::push(const std::vector<std::unique_ptr<Term> > &terms) {
  for (auto &x : terms) scope.emplace_back(x.get());
}

std::unique_ptr<Term> Term::optimize(std::unique_ptr<Term> term) {
  term = Term::pass_purity(std::move(term));
  term = Term::pass_usage (std::move(term));
  term = Term::pass_sweep (std::move(term));
  term = Term::pass_inline(std::move(term));
  return term;
}
