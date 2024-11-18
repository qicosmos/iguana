# Instructions for using the .proto file to struc_pack header tool

## compile

libprotobuf and libprotoc version is 3.21.0.

```shell
mkdir build
cd build
cmake .. && make
```

## usage

Usage:

```shell
protoc --plugin=protoc-gen-custom=./build/proto_to_struct  data.proto --custom_out=:./protos
```

data.proto is the original file that is intended to be the structure pack file.

`--custom_out=` is followed by the path to the generated file.

data.proto:

```proto
syntax = "proto3";

package mygame;

option optimize_for = SPEED;
option cc_enable_arenas = true;

message Vec3 {
    float x = 1;
    float y = 2;
    float z = 3;
}

message Weapon {
    string name = 1;
    int32 damage = 2;
}

message Monster {
  Vec3 pos = 1;
  int32 mana = 2;
  int32 hp = 3;
  string name = 4;
  bytes inventory = 5;
  enum Color {
        Red = 0;
        Green = 1;
        Blue = 2;
  }
  Color color = 6;
  repeated Weapon weapons = 7;
  Weapon equipped = 8;
  repeated Vec3 path = 9;
}

message Monsters {
    repeated Monster monsters = 1;
}

message person {
    int32 id = 1;
    string name = 2;
    int32 age = 3;
    double salary = 4;
}

message persons {
    repeated person person_list = 1;
}

message bench_int32 {
    int32 a = 1;
    int32 b = 2;
    int32 c = 3;
    int32 d = 4;
}
```

generate struct pack file:

```cpp
#pragma once
#include <ylt/struct_pb.hpp>

enum class Color {
	Red = 0,
	Green = 1,
	Blue = 2,
};

struct Vec3 {
	float x;
	float y;
	float z;
};
YLT_REFL(Vec3, x, y, z);

struct Weapon {
	std::string name;
	int32_t damage;
};
YLT_REFL(Weapon, name, damage);

struct Monster {
	Vec3 pos;
	int32_t mana;
	int32_t hp;
	std::string name;
	std::string inventory;
	Color color;
	std::vector<Weapon> weapons;
	Weapon equipped;
	std::vector<Vec3> path;
};
YLT_REFL(Monster, pos, mana, hp, name, inventory, color, weapons, equipped, path);

struct Monsters {
	std::vector<Monster> monsters;
};
YLT_REFL(Monsters, monsters);

struct person {
	int32_t id;
	std::string name;
	int32_t age;
	double salary;
};
YLT_REFL(person, id, name, age, salary);

struct persons {
	std::vector<person> person_list;
};
YLT_REFL(persons, person_list);

struct bench_int32 {
	int32_t a;
	int32_t b;
	int32_t c;
	int32_t d;
};
YLT_REFL(bench_int32, a, b, c, d);


```

There are two parameters:

## add_optional

Generate C++ files in optional struct pack format.

```shell
protoc --plugin=protoc-gen-custom=./build/proto_to_struct  data.proto --custom_out=add_optional:./protos
```

## enable_inherit

Generate C++ files in non std::optional format and the file conforms to the `struct pb` standard.

```shell
protoc --plugin=protoc-gen-custom=./build/proto_to_struct  data.proto --custom_out=enable_inherit:./protos

```

## add_optional and enable_inherit

```shell
protoc --plugin=protoc-gen-custom=./build/proto_to_struct  data.proto --custom_out=add_optional+enable_inherit:./protos
```
