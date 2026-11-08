////
// Copyright (C) 2026 Sony Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
////

#pragma once

#include <cstddef>
#include <string>


typedef void* LibHandle;

typedef bool (*IsMmfBytesProc) (int bytes_size, std::byte* bytes);

typedef bool (*IsSkeletonDefinitionBytesProc) (int bytes_size, std::byte* bytes);

typedef bool (*IsFrameDataBytesProc) (int bytessize, std::byte* bytes);

typedef bool (*ConvertBytesToSkeletonDefinitionProc) (
  int bytes_size, std::byte* bytes,
  unsigned long long& sender_ip, int& sender_port,
  int& data_size, int*& joint_ids, int*& parent_joint_ids,
  float*& rotations_x, float*& rotations_y, float*& rotations_z, float*& rotations_w,
  float*& positions_x, float*& positions_y, float*& positions_z
  );

typedef bool (*ConvertBytesToFrameDataProc) (
  int bytes_size, std::byte* bytes,
  unsigned long long& sender_ip, int& sender_port,
  int& frame_number, float& timestamp, double& utc_time,
  uint8_t& timecode_hour, uint8_t& timecode_min, uint8_t& timecode_sec, uint8_t& timecode_frame,
  uint8_t& timecode_frame_rate, bool& timecode_drop_frame,
  int& data_size, int*& joint_ids,
  float*& rotations_x, float*& rotations_y, float*& rotations_z, float*& rotations_w,
  float*& positions_x, float*& positions_y, float*& positions_z
  );

class MocopiMotionFormat
{
public:

  static void Initialize(const std::string& pluginPath);
  static void Shutdown();
  static bool IsValid();

protected:
  // DLL Functions
  static IsMmfBytesProc IsMmfBytes;
  static IsSkeletonDefinitionBytesProc IsSkeletonDefinitionBytes;
  static IsFrameDataBytesProc IsFrameDataBytes;
  static ConvertBytesToSkeletonDefinitionProc ConvertBytesToSkeletonDefinition;
  static ConvertBytesToFrameDataProc ConvertBytesToFrameData;

private:

  static LibHandle mDllHandle;

  static std::string mPluginPath;

  static bool LoadDll();
  static std::string GetDLLPath();

};

