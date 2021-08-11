/**
 * @file instance_extensions.hpp
 * @copyright 2021-2021 Leon Brands. All rights served.
 * @license: https://github.com/Legion-Engine/Legion-LLRI/blob/main/LICENSE
 */

#pragma once

namespace LLRI_NAMESPACE
{
    /**
     * @brief Describes the kind of instance extension. <br>
     * This value is used in instance_extension and is used in :func:`llri::createInstance()` to recognize which value to pick from instance_extension's union. <br>
     * <br>
     * Instance Extensions aren't guaranteed to be available so use this enum with llri::queryInstanceExtensionSupport() to find out if your desired extension is available prior to adding the extension to your instance desc extension array.
    */
    enum struct instance_extension_type
    {
        /**
         * @brief Validate API calls, their parameters, and context.
         */
        APIValidation,
        /**
         * @brief Validate shader operations such as buffer reads/writes.
        */
        GPUValidation,
        /**
         * @brief The highest value in this enum.
        */
        MaxEnum = GPUValidation
    };

    /**
     * @brief Converts an instance_extension_type to a string to aid in debug logging.
    */
    constexpr const char* to_string(const instance_extension_type& result);

    /**
     * @brief Enable or disable API-side validation.
     * API validation checks for parameters and context validity and sends the appropriate messages back if the usage is invalid or otherwise concerning.
    */
    struct api_validation_ext
    {
        bool enable : 1;
    };

    /**
     * @brief Enable or disable GPU-side validation.
     * GPU validation validates shader operations such as buffer read/writes. Enabling this can be useful for debugging but is often associated with a significant cost.
    */
    struct gpu_validation_ext
    {
        bool enable : 1;
    };

    /**
     * @brief Describes an instance extension with its type.
     *
     * Instance extensions are additional features that are injected into the instance. They **may** activate custom behaviour in the instance, or they **may** enable the user to use functions or structures related to the extension.
     *
     * The support for each available instance_extension is fully **optional** (hence their name, extension), and thus before enabling any instance_extension, you should first query its support with queryInstanceExtensionSupport().
    */
    struct instance_extension
    {
        /**
         * @brief The type of instance extension.
         *
         * As instance extensions must be passed in an array, they need some way of storing varying data contiguously. instance_extensions do so through an unnamed union. When llri::createInstance() attempts to implement the extension, it uses this enum value to determine which union member to pick from.
         *
         * @note Accessing incorrect union members is UB, so the union member you set **must** match with the instance_extension_type passed.
        */
        instance_extension_type type;

        /**
         * @brief The instance extension's information. <br>
         * One of the values in this union must be set, and that value must be the same value as the given type. <br>
         * Passing a different structure than expected causes undefined behaviour and may result in unexplainable result values or implementation errors. <br>
         * All instance extensions are named after their enum entry, followed with EXT or _ext (e.g. instance_extension_type::APIValidation is represented by api_validation_ext).
        */
        union
        {
            api_validation_ext apiValidation;
            gpu_validation_ext gpuValidation;
        };

        instance_extension() = default;
        instance_extension(const instance_extension_type& type, const api_validation_ext& ext) : type(type), apiValidation(ext) { }
        instance_extension(const instance_extension_type& type, const gpu_validation_ext& ext) : type(type), gpuValidation(ext) { }
    };

    namespace detail
    {
        [[nodiscard]] bool queryInstanceExtensionSupport(const instance_extension_type& type);
    }

    /**
     * @brief Queries the support of the given extension.
     * @return true if the extension is supported, and false if it isn't.
     */
    [[nodiscard]] bool queryInstanceExtensionSupport(const instance_extension_type& type);
}
