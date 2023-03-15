# Changelog

## 1.1

* Added extra constructors to PNGPayload to match the constructors of png::Image.
* Added support for creating payloads out of icon files with embedded PNG images in them.

## 1.0

* Initial release!
  * Created a basic PNG encoding library with definitive support for TrueColor and AlphaTrueColor pixels and theoretical support for other pixel types.
  * Created an ability to create arbitrary PNG payloads:
    * Trailing data method
    * `tEXt` section method
    * `zTXt` section method
    * least-significant bit steganography method
