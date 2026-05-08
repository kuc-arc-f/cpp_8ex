# http_1

 Version: 0.9.1

 date    : 2026/05/08

 update :

***

C++ Linux LLVM CLang , todo API server

* LLVM CLang use
***

### setup

* json-LIB

```
sudo apt install nlohmann-json3-dev
```
***
### related

https://github.com/yhirose/cpp-httplib/blob/master/httplib.h


***
* httplib.h
```
wget https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.43.2.tar.gz
tar xvf v0.43.2.tar.gz
cp cpp-httplib-0.43.2/httplib.h  .
rm -fr cpp-httplib-0.43.2/
```

***
* build

```
make all
```
***
* start
```
./server
```


***
* test-data

* add
```
curl -X POST http://localhost:8000/todos \
     -H "Content-Type: application/json" \
     -d '{"title": "tit-21"}'
```

* DELETE
```
curl -X DELETE http://localhost:8000/todos/3
```

* list
```
curl http://localhost:8000/todos
```
***
