// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*!
 * @file NetboxMessage1.h
 * This header file contains the declaration of the described types in the IDL file.
 *
 * This file was generated by the tool fastddsgen.
 */

#ifndef _FAST_DDS_GENERATED_NETBOXMESSAGE1_H_
#define _FAST_DDS_GENERATED_NETBOXMESSAGE1_H_

#include <array>
#include <bitset>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <fastcdr/cdr/fixed_size_string.hpp>
#include <fastcdr/xcdr/external.hpp>
#include <fastcdr/xcdr/optional.hpp>



#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#define eProsima_user_DllExport __declspec( dllexport )
#else
#define eProsima_user_DllExport
#endif  // EPROSIMA_USER_DLL_EXPORT
#else
#define eProsima_user_DllExport
#endif  // _WIN32

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#if defined(NETBOXMESSAGE1_SOURCE)
#define NETBOXMESSAGE1_DllAPI __declspec( dllexport )
#else
#define NETBOXMESSAGE1_DllAPI __declspec( dllimport )
#endif // NETBOXMESSAGE1_SOURCE
#else
#define NETBOXMESSAGE1_DllAPI
#endif  // EPROSIMA_USER_DLL_EXPORT
#else
#define NETBOXMESSAGE1_DllAPI
#endif // _WIN32

namespace eprosima {
namespace fastcdr {
class Cdr;
class CdrSizeCalculator;
} // namespace fastcdr
} // namespace eprosima







/*!
 * @brief This class represents the structure NetboxMessage1 defined by the user in the IDL file.
 * @ingroup NetboxMessage1
 */
class NetboxMessage1
{
public:

    /*!
     * @brief Default constructor.
     */
    eProsima_user_DllExport NetboxMessage1();

    /*!
     * @brief Default destructor.
     */
    eProsima_user_DllExport ~NetboxMessage1();

    /*!
     * @brief Copy constructor.
     * @param x Reference to the object NetboxMessage1 that will be copied.
     */
    eProsima_user_DllExport NetboxMessage1(
            const NetboxMessage1& x);

    /*!
     * @brief Move constructor.
     * @param x Reference to the object NetboxMessage1 that will be copied.
     */
    eProsima_user_DllExport NetboxMessage1(
            NetboxMessage1&& x) noexcept;

    /*!
     * @brief Copy assignment.
     * @param x Reference to the object NetboxMessage1 that will be copied.
     */
    eProsima_user_DllExport NetboxMessage1& operator =(
            const NetboxMessage1& x);

    /*!
     * @brief Move assignment.
     * @param x Reference to the object NetboxMessage1 that will be copied.
     */
    eProsima_user_DllExport NetboxMessage1& operator =(
            NetboxMessage1&& x) noexcept;

    /*!
     * @brief Comparison operator.
     * @param x NetboxMessage1 object to compare.
     */
    eProsima_user_DllExport bool operator ==(
            const NetboxMessage1& x) const;

    /*!
     * @brief Comparison operator.
     * @param x NetboxMessage1 object to compare.
     */
    eProsima_user_DllExport bool operator !=(
            const NetboxMessage1& x) const;

    /*!
     * @brief This function sets a value in member id
     * @param _id New value for member id
     */
    eProsima_user_DllExport void id(
            int64_t _id);

    /*!
     * @brief This function returns the value of member id
     * @return Value of member id
     */
    eProsima_user_DllExport int64_t id() const;

    /*!
     * @brief This function returns a reference to member id
     * @return Reference to member id
     */
    eProsima_user_DllExport int64_t& id();


    /*!
     * @brief This function copies the value in member topics
     * @param _topics New value to be copied in member topics
     */
    eProsima_user_DllExport void topics(
            const std::vector<std::string>& _topics);

    /*!
     * @brief This function moves the value in member topics
     * @param _topics New value to be moved in member topics
     */
    eProsima_user_DllExport void topics(
            std::vector<std::string>&& _topics);

    /*!
     * @brief This function returns a constant reference to member topics
     * @return Constant reference to member topics
     */
    eProsima_user_DllExport const std::vector<std::string>& topics() const;

    /*!
     * @brief This function returns a reference to member topics
     * @return Reference to member topics
     */
    eProsima_user_DllExport std::vector<std::string>& topics();


    /*!
     * @brief This function sets a value in member timestamp
     * @param _timestamp New value for member timestamp
     */
    eProsima_user_DllExport void timestamp(
            uint64_t _timestamp);

    /*!
     * @brief This function returns the value of member timestamp
     * @return Value of member timestamp
     */
    eProsima_user_DllExport uint64_t timestamp() const;

    /*!
     * @brief This function returns a reference to member timestamp
     * @return Reference to member timestamp
     */
    eProsima_user_DllExport uint64_t& timestamp();


    /*!
     * @brief This function copies the value in member payload
     * @param _payload New value to be copied in member payload
     */
    eProsima_user_DllExport void payload(
            const std::vector<uint8_t>& _payload);

    /*!
     * @brief This function moves the value in member payload
     * @param _payload New value to be moved in member payload
     */
    eProsima_user_DllExport void payload(
            std::vector<uint8_t>&& _payload);

    /*!
     * @brief This function returns a constant reference to member payload
     * @return Constant reference to member payload
     */
    eProsima_user_DllExport const std::vector<uint8_t>& payload() const;

    /*!
     * @brief This function returns a reference to member payload
     * @return Reference to member payload
     */
    eProsima_user_DllExport std::vector<uint8_t>& payload();

private:

    int64_t m_id{0};
    std::vector<std::string> m_topics;
    uint64_t m_timestamp{0};
    std::vector<uint8_t> m_payload;

};

#endif // _FAST_DDS_GENERATED_NETBOXMESSAGE1_H_



