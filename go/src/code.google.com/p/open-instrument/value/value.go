package value

import (
  oproto "code.google.com/p/open-instrument/proto"
  "sort"
)

type By func(p1, p2 *oproto.Value) bool

func (by By) Sort(values []*oproto.Value) {
  sfs := &valuesSorter{
    values: values,
    by:     by,
  }
  sort.Sort(sfs)
}

type valuesSorter struct {
  values []*oproto.Value
  by     By
}

func (this *valuesSorter) Len() int {
  return len(this.values)
}

func (this *valuesSorter) Swap(i, j int) {
  this.values[i], this.values[j] = this.values[j], this.values[i]
}

func (this *valuesSorter) Less(i, j int) bool {
  return this.by(this.values[i], this.values[j])
}
