# http_8

 Version: 0.9.1

 date    : 2026/05/08

 update :

***

C++ Linux LLVM CLang , C++ React todo server

* LLVM CLang use
* SQLite DB
* node 22

***
### related

* https://github.com/nlohmann/json

* https://github.com/yhirose/cpp-httplib/blob/master/httplib.h

***

### setup

* LIB

```
sudo apt install nlohmann-json3-dev
sudo apt-get install libsqlite3-dev
```
***
* dotenv-cpp

```
git clone https://github.com/laserpants/dotenv-cpp.git
mkdir -p include
cp -rn <path-to-this-repo>/dotenv-cpp/include/ .
```

***
* httplib.h
```
wget https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.43.2.tar.gz
tar xvf v0.43.2.tar.gz
cp cpp-httplib-0.43.2/httplib.h  include/
rm -fr cpp-httplib-0.43.2/
```
***

```
$ tree include/
include/
├── httplib.h
├── laserpants
│   └── dotenv
│       └── dotenv.h
├── my_page.hpp
└── my_todo.hpp
```

***
* front-build

```
npm i
npm run build
```

***
### .env
```
VALID_LOGIN=true
USER_NAME=user123@example.com
PASSWORD=123
```

***
* build
```
make all
```

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
