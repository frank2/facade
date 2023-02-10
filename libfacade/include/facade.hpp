#ifndef __FACADE_HPP
#define __FACADE_HPP

//! @mainpage Facade: A PNG Payload Library and Tool
//!
//! **Facade** is a library and tool that can manipulate PNG files to include arbitrary payloads inside of them.
//! There are many techniques that can be employed with the binary, but further functionality can be extended
//! through use of the tool's library. For example, all known pixel types for PNG files are technically supported,
//! but functionality still needs to be added to fully integrate them into the payload process. *Facade* actually
//! comes with a basic PNG encoder under the hood that theoretically supports all pixel types!
//!
//! A basic way to get started with adding payloads via the library is the facade::PNGPayload class:
//! @include payload_creation.cpp
//!
//! And extracting this data from a known image is just as easy:
//! @include payload_extraction.cpp
//!
//! Note that these examples in particular assume that the payload data is already there! If it isn't, you may encounter
//! exceptions, which you can view in exception.hpp. For more granular control of the individual PNG files, see
//! facade::PNGPayload's base class, facade::png::Image.
//!
//! For further reading about the PNG format, check out [this blog post](https://www.da.vidbuchanan.co.uk/blog/hello-png.html)
//! by David Buchanan! To understand the design intent of this library, check out @ref design "the design section"! For
//! manipulating the PNG image directly (e.g., for writing your own steganography code), check out
//! @ref usage "the usage section"!
//!

#include <facade/platform.hpp>
#include <facade/utility.hpp>
#include <facade/png.hpp>
#include <facade/payload.hpp>

#endif
