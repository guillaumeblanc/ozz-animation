---
title: Advanced
layout: full
keywords: advanced,multi-threading,io,memory,management,serialization,openmp,maths,soa,sse,simd
order: 70
---

{% include links.jekyll %}

Thread safety
=============
ozz run-time library are thread safe, and was design as such from the ground. Ozz does not impose a way to distribute the workload across different threads, but rather propose thread-safe data structures and processing unit, that can be distributed. This data-oriented approach makes everything clear about thread safety:

- Jobs (aka processing units: sampling, blending...) are thread safe.
- The user is in charge of allocating and specifying jobs' inputs/outputs. He ensures that there's no race conditions in the way he uses memory and run jobs.

The [multi-threading sample][link_multithread_sample] demonstrates how ozz-animation runtime can (naively yet safely) be used in a multi-threaded context, using openmp to distribute jobs execution across threads.

File IO management
==================
IO operations in ozz are done through the `ozz::io::Stream` interface, which conforms to crt FILE API.
ozz propose two implementations of the Stream interface:

- `ozz::IO::File`: Allows to read and write to a file. Opening is done at construction time, using the same API as the crt fopen function. The File destructor automatically closes the file if it was opened. 
- `ozz::IO::MemoryStream`: Implements an in-memory Stream. It allows to seamlessly use a memory buffer as a Stream. The opening mode is equivalent to fopen w+b (binary read/write).

One can implement `ozz::io::Stream` interface to remap IO operations to his own IO management strategy. This is useful for example to create a read or write stream mapped to a sub-part of a file (or a package) that contains many other data but ozz data. Overloading `ozz::io::Stream` interface requires to implement the following functions:
{% highlight cpp %}
  // Tests whether a file is opened.
  virtual bool opened() const = 0;

  // Reads _size bytes of data to _buffer from the stream. _buffer must be big
  // enough to store _size bytes. The position indicator of the stream is
  // advanced by the total amount of bytes read.
  // Returns the number of bytes actually read, which may be less than _size.
  virtual std::size_t Read(void* _buffer, std::size_t _size) = 0;

  // Writes _size bytes of data from _buffer to the stream. The position
  // indicator of the stream is advanced by the total number of bytes written.
  // Returns the number of bytes actually written, which may be less than _size.
  virtual std::size_t Write(const void* _buffer, std::size_t _size) = 0;

  // Sets the position indicator associated with the stream to a new position
  // defined by adding _offset to a reference position specified by _origin.
  // Returns a zero value if successful, otherwise returns a non-zero value.
  virtual int Seek(int _offset, Origin _origin) = 0;

  // Returns the current value of the position indicator of the stream.
  // Returns -1 if an error occurs.
  virtual int Tell() const = 0;
{% endhighlight %}

Serialization mechanics
=======================
Serialization is based on the concept of archives. Archives are similar to c++ `iostream`. Data can be saved to a OArchive with the `<< operator`, or loaded from a IArchive with the `>> operator`. Archives read or write data to `ozz::io::Stream` objects. 

Primitive data types are simply saved/loaded to/from archives, while struct and class are saved/loaded through Save/Load intrusive or non-intrusive functions:

- The intrusive function prototypes are:
{% highlight cpp %}
  void ObjectType::Save(ozz::io::OArchive*) const;
  void ObjectType::Load(ozz::io::IArchive*);
{% endhighlight %}
- The non-intrusive functions allow to work on arrays of objects, which is very important for optimization. They must be implemented in ozz::io namespace with these prototypes:
{% highlight cpp %}
  namespace ozz {
  namespace io {
  void Save(OArchive& _archive, const _Ty* _ty, std::size_t _count);
  void Load(IArchive& _archive, _Ty* _ty, std::size_t _count);
  }  // io
  }  // ozz
{% endhighlight %}

Serializing arrays
------------------
Arrays of struct/class or primitive types can be saved/loaded with the helper function `ozz::io::MakeArray()` which is then streamed in or out using `<<` and `>>` archive operators:
{% highlight cpp %}
archive << ozz::io::MakeArray(my_array, count);
{% endhighlight %}

Versioning
----------
Versioning can be done using `OZZ_IO_TYPE_VERSION` macros. Type version is saved in the OArchive, and is given back to Load functions to allow to manually handle version modifications.
{% highlight cpp %}
namespace ozz {
namespace io {
OZZ_IO_TYPE_VERSION(1, _ObjectType_)
}  // io
}  // ozz
{% endhighlight %}
Versioning can be disabled using `OZZ_IO_TYPE_NOT_VERSIONABLE` like macros. It can be used to optimize native/simple object types. It can not be re-enabled afterward without braking version compatibility.

Tagging
-------
Objects can be assigned a tag (a string) using OZZ_IO_TYPE_TAG macros. A tag allows to check the type of the object to read from an archive.
{% highlight cpp %}
namespace ozz {
namespace io {
OZZ_IO_TYPE_TAG("_tag_", _ObjectType_)
}  // io
}  // ozz
{% endhighlight %}
When reading from an IArchive, an automatic assertion check is performed for each object that has a tag declared. The check can also be done manually, before actually reading and object, to ensure an archive contains the expected object type.
{% highlight cpp %}
archive.TestTag<_ObjectType_>()
{% endhighlight %}

Endianness
----------
Endianness (big or little endian) can be specified while constructing an output archive (ozz::io::OArchive). Input archives automatically handle endianness conversion if the native platform endian mode differs from the archive one.

Error management
================
IArchive and OArchive expect valid streams as argument, respectively opened for reading and writing. Archives do NOT perform error detection while reading or writing. All errors are considered programming errors. This leads to the following assertions on the user side:

- When writing: the stream must be big (or grow-able) enough to support the data being written.
- When reading: Stream's tell (position in the stream) must match the object being read. To help with this requirement, archives provide a tag mechanism that allows to check the tag (ie the type) of the next object to read. Stream integrity, like data corruption or file truncation, must also be validated on the user side, using a CRC for example.

Memory management
=================
Dynamic memory allocations, from any ozz library, all go through a custom allocator. This allocator can be used to redirect all allocations to your own memory manager, defining your own memory management strategy.
To redirect memory allocations, override ozz::memory::Allocator object and implement its interface:
{% highlight cpp %}
  // Allocates _size bytes on the specified _alignment boundaries.
  // Allocate function conforms with standard malloc function specifications.
  virtual void* Allocate(std::size_t _size, std::size_t _alignment) = 0;

  // Frees a block that was allocated with Allocate or Reallocate.
  // Argument _block can be NULL.
  // Deallocate function conforms with standard free function specifications.
  virtual void Deallocate(void* _block) = 0;

  // Changes the size of a block that was allocated with Allocate.
  // Argument _block can be NULL.
  // Reallocate function conforms with standard realloc function specifications.
  virtual void* Reallocate(void* _block,
                           std::size_t _size,
                           std::size_t _alignment) = 0;	
{% endhighlight %}

... and replace ozz default allocator with your own, using ozz::memory::SetDefaulAllocator() function.

Maths
=====
ozz provides different math libraries, depending on how math data are intended to be used and processed:

Fpu-based library
-----------------
Provides math objects (vectors, primitives...) and operations. These are intended to be used when access simplicity and storage size are more important than algorithm performance. The ozz::math::Float3 (x, y, z components vector) math objects for example provides a simple access to each of its components (x, y and z), simple initialization functions and constructors, simple and usual math/geometric operations (additions, cross-product...) which make it very simple to use. On the other end the implementation relies on fpu scalar operations, with no vectorial (SIMD, SSE...) optimizations.
The following files are provided:

- ozz/base/maths/math_ex.h: math.h extension with min, max, clamp sort of operations.
- ozz/base/maths/math_constant.h: Math trigonometric constants definition.
- ozz/base/maths/vec_float.h: Math 2, 3 or 4 components vectors (ozz::math::Float2, ozz::math::Float3, ozz::math::Float4), with usual geometric operations. 
- ozz/base/maths/quaternion.h: ozz::math::Quaternion implementation.
- ozz/base/maths/transform.h: ozz::math::Transform affine transformation decomposition. Translation (ozz::math::Float3), rotation (ozz::math::Quaternion) and scale (ozz::math::Float3) are all stored as separate members in ozz::math::Transform. This structure has math operations and conversions utilities.
- ozz/base/maths/box.h: 3 dimensional floating-point box (ozz::math::Box). 
- ozz/base/maths/rect.h: 2 dimensional floating-point and integer rectangles (ozz::math::RectFloat, ozz::math::RectInt).

SIMD math library
-----------------
Provides SIMD optimized math objects (matrices, vectors...) and operations, intended to be used when performance is important and most of all data access pattern cope with SIMD restrictions:

- 16 bytes minimum alignment.
- Minimum use of constants.
- Minimum per component access, load and store operation of a SIMD element.
- Minimum horizontal instructions, aka inter component operations inside a SIMD value, like dot product for example.
- ...

This library is implemented in a single file ozz/base/maths/simd_math.h which provides the following math structure:

- ozz::math::SimdFloat4 structure: 4 components vector and math operations.
- ozz::math::simd_float4 namespace: Initialization (load) functions for SimdFloat4 objects, and constants (one, zero, x_axis...).
- ozz::math::SimdInt4 structure: 4 components vector, math and bitwise operations.
- ozz::math::simd_int4 namespace: Initialization (load) functions for SimdInt4 objects, and constants (one, zero, x_axis...).
- ozz::math::Float4x4 structure: 4 by 4 floating point matrix structure and operations.

Current version has SSE2 to SSE4 implementation, and a reference fpu one used as fallback when SSE isn't available. Adding other SIMD implementation (VMX...) is doable thanks to the complete existing unit-tests suite.

SoA SIMD math library
---------------------
This library provides math objects and operations with an SoA (Struct Of Array) data layout. That means that every component of a vector for example, is in fact an array of 4 values (standard SIMD size). A single ozz::math::SoaFloat3 is in fact storing 4 vectors in the form xxxx, yyyy and zzzz. Data access simplicity is of course limited, but performance benefit is important because this data layout removes some of the SIMD constraints:

- Math objects components can be accessed at will as they are different registers.
- SIMD horizontal operations are possible as vector components are stored on different SIMD registers.
- All 4 elements of the SIMD register are always used, which is not the case when using a SIMD to store a 3D vector.

Furthermore instruction throughput is maximized over AoS (Array of Struct) as dependencies between  instructions during math standard operations is reduced.
The drawback is that it's impossible to execute conditional operations on one element of the array inside a SoA data (as they are SIMD registers). The solution in this case is to execute operations for the 2 code paths of the condition and use masks to compose the result (see ozz::math::Select functions). SoA comparison expressions returns ozz::math::SimdInt4 values (instead of bools for AoS) which can directly be used for masking or Select like functions.
This library relies on these following files to provides SoA structures similar with the fpu-based library:

- ozz/base/maths/soa_float.h: Math 2, 3 or 4 components SoA vectors (ozz::math::SoAFloat2, ozz::math::SoAFloat3, ozz::math::SoAFloat4), with usual geometric operations. 
- ozz/base/maths/soa_float4x4.h: A Soa 4 by 4 matrix implementation ozz::math::SoaFloat4x4.
- ozz/base/maths/soa_quaternion.h: A Soa quaternion implementation ozz::math::SoaQuaternion.
- ozz/base/maths/soa_transform.h: A Soa affine transformation  ozz::math::SoaTransform.

The library is implemented over the SIMD math library and takes all the benefits of SSE or other SIMD implementations.
ozz-animation make an heavy use of SoA data structures (see ozz::animation::BlendingJob for example), which could be used as examples. Data oriented programming is very important to maximize benefits of SoA structures, allowing to arrange the data according to the processing that's operating on them.

Containers
==========

Standard containers
-------------------

ozz base library remaps stl standard containers ([vectors, maps..][link_src_containers]) to ozz memory allocators. The purpose is to track memory allocations and avoid leaks.

> Note that these containers are not used by the runtime libraries.

Custom containers
-----------------

Base library implement an linked node list ([`ozz::container::IntrusiveList`][link_src_intrusive_list]), with the benefit of many O1 algorithms and a different memory scheme compared to std::list. It's templatized for easier reuse and compatible with std::list API and stl algorithms.

Options
=======

This library implements a command line option processing utility. It helps with command line parsing by converting arguments to c++ objects of type bool, int, float or c-string.
Unlike `getogt()`, program options can be scattered in the source files (a la google-gflags). Options are collected by a parser which then automatically generate the help/usage screen based on registered options.
This library is made of a single .h and .cc with no other dependency, to make it simple to be reused.

To set an option from the command line, use the form *\-\-option=value* for non-boolean options, and *\-\-option/--nooption* for booleans. For example, *\-\-var=46* will set *var* variable to 46. If *var* type is not compatible with the specified argument type (in this case an integer, a float or a string), then the parser displays the help message and requires application to exit.

Boolean options can be set using different syntax:

- to set a boolean option to true: *\-\-var*, *\-\-var=true*, *\-\-var=t*, *\-\-var=yes*, *\-\-var=y*, *\-\-var=1*.
- to set a boolean option to false: *\-\-novar*, *\-\-var=false*, *\-\-var=f*, *\-\-var=no*, *\-\-var=n*, *\-\-var=0*.
Consistently using single-form --option/--nooption is recommended though.

Specifying an option (in the command line) that has not been registered is an error, the parsing library will request the application to exit.

As in `getopt()` and `gflags`, -- by itself terminates flags processing. So in: *foo \-\-f1=1 \-\- \-\-f2=2*, *f1* is considered but *f2* is not.

Parsing is invoked through `ozz::options::ParseCommandLine()` function, providing argc and argv arguments of the main function. This function also takes as argument two strings to specify the version and usage message.

To declare/register a new option, use `OZZ_OPTIONS_DECLARE_xxx` like macros. Supported options types are bool, int, float and string (c string).
`OZZ_OPTIONS_DECLARE_xxx` macros arguments allow to give the option a:

- name, used in the code to read option value.
- description, used by the auto-generated help screen.
- default value.
- required flag, that specifies if the option is optional.

So for example, in order to define a boolean "verbose" option, that is false by default and optional (ie: not required):
{% highlight cpp %}
OZZ_OPTIONS_DECLARE_BOOL(verbose, "Display verbose output", false, false);
{% endhighlight %}
This option can then be referenced from the code using `OPTIONS_verbose` c++ global variable, that implement an automatic cast operator to the option's type (bool in this case).

The parser also integrates built-in options:

- *\-\-help* displays the help screen, that is automatically generated based on the registered options.
- *\-\-version* displays executable's version.
