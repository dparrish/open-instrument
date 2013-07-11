package variable

import (
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "encoding/csv"
  "errors"
  "fmt"
  "log"
  "regexp"
  "sort"
  "strings"
)

// Variable represents a variable name with all labels.
type Variable struct {
  Variable string
  Labels   map[string]string
}

func valueNeedsQuotes(str string) bool {
  matched, _ := regexp.MatchString("^[,}]*$", str)
  return matched
}

func sortedKeys(m map[string]string) []string {
  keys := make([]string, len(m))
  i := 0
  for k := range m {
    keys[i] = k
    i++
  }
  sort.Strings(keys)
  return keys
}

// String returns the complete variable as a string, including all labels correctly quoted.
func (v Variable) String() string {
  str := v.Variable
  if len(v.Labels) > 0 {
    str += "{"
    first := true
    labels := sortedKeys(v.Labels)
    for _, key := range labels {
      value := v.Labels[key]
      if !first {
        str += ","
      }
      first = false
      str += key
      str += "="
      needs_quotes := valueNeedsQuotes(value)
      if needs_quotes {
        str += "\""
      }
      str += value
      if needs_quotes {
        str += "\""
      }
    }
    str += "}"
  }
  return str
}

// AsProto returns the Varabile encoded as a new StreamVariable protobuf.
func (v Variable) AsProto() (p *openinstrument_proto.StreamVariable) {
  p = new(openinstrument_proto.StreamVariable)
  v.ToProto(p)
  return
}

// AsProto encodes the Variable into an existing protobuf.
func (v Variable) ToProto(p *openinstrument_proto.StreamVariable) {
  p.Reset()
  p.Name = proto.String(v.Variable)
  p.Label = make([]*openinstrument_proto.Label, len(v.Labels))
  var i int
  for key, value := range v.Labels {
    p.Label[i] = &openinstrument_proto.Label{Label: proto.String(key), Value: proto.String(value)}
    i++
  }
}

// ParseFromString extracts the variable name and all labels from a string.
func (v *Variable) ParseFromString(textvar string) error {
  re, err := regexp.Compile("^(.+?)({(.*)})?$")
  if err != nil {
    log.Panic("Invalid regexp")
  }
  matches := re.FindStringSubmatch(textvar)
  v.Variable = matches[1]
  if matches[3] != "" {
    reader := csv.NewReader(strings.NewReader(matches[3]))
    record, err := reader.Read()
    if err != nil {
      return errors.New(fmt.Sprintf("Error parsing labels from %s: %s", textvar, err))
    }
    v.Labels = make(map[string]string, 0)
    for _, item := range record {
      substrings := strings.SplitN(item, "=", 2)
      if len(substrings) != 2 {
        return errors.New(fmt.Sprintf("Error splitting labels from %s", textvar))
      }
      key, value := substrings[0], substrings[1]
      v.Labels[key] = value
    }
  }
  return nil
}

// ParseFromProto extracts the details from a protobuf.
func (v *Variable) ParseFromProto(p *openinstrument_proto.StreamVariable) error {
  v.Variable = *p.Name
  // Copy labels
  v.Labels = make(map[string]string, len(p.Label))
  for _, label := range p.Label {
    v.Labels[label.GetLabel()] = label.GetValue()
  }
  return nil
}

// Match checks that the supplied variable matches this one, using the matching rules defined above.
func (v *Variable) Match(match *Variable) bool {
  if strings.HasSuffix(match.Variable, "*") {
    m := match.Variable[0 : len(match.Variable)-2]
    if !strings.HasPrefix(v.Variable, m) {
      // Invalid prefix match
      return false
    }
  } else if match.Variable != v.Variable {
    // Variable name does not match
    return false
  }
  for key, value := range match.Labels {
    if value == "" {
      // Key must not exist
      _, ok := v.Labels[key]
      if ok {
        return false
      }
    } else if value == "*" {
      // Key must exist
      val, ok := v.Labels[key]
      if !ok {
        return false
      }
      if val == "" {
        return false
      }
    } else {
      val, ok := v.Labels[key]
      if !ok {
        return false
      }
      if val != value {
        return false
      }
    }
  }
  return true
}

// NewFromString creates a new Variable from the supplied string.
func NewFromString(textvar string) *Variable {
  v := new(Variable)
  if err := v.ParseFromString(textvar); err != nil {
    log.Println(err)
    return nil
  }
  return v
}

// NewFromProto creates a new Variable from the supplied protobuf.
func NewFromProto(p *openinstrument_proto.StreamVariable) *Variable {
  v := new(Variable)
  if err := v.ParseFromProto(p); err != nil {
    log.Println(err)
    return nil
  }
  return v
}
