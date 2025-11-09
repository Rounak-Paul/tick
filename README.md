IMPORTANT: do not modify this document. 

tick is a programming language, which is a default parallel language for CPU. Applications built with tick will have no race condition or deadlock, its a true parallel first architechure. tick is a strongly typed general perpous programming language. tick applications will exhibit complete utilization of available CPU resource in a automatic fashion, users will write the application as an example. the language should be like semi compiled, semi interpreted like python:

tick```
event e1;
event e2;

signal<int> int_sig1;
signal<int> int_sig2;

@e1
process p1 {
  // do something
}

@e1
process p2 {
  // do something
  int_sig2.emmit(// something)
}

int some_func(int a) {
  int x = int_sig2.recv();
  return x + a;
}

@e2
process p3 {
  int a = 0;
  int b = some_func(a, 23);
  print(b);
  int_sig1.emmit(b);
}

int main() {
  e1.execute() // processes connected to e1 will execute in parallel.

  while true {
    sig2.execute()
    if (int_sig1.recv() == 128) {
      break;
    }
  }
}

```

things like while, for loops, etc are not parallel, its a vhdl like language but for cpu and general perpous. 