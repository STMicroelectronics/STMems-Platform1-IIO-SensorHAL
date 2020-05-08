/*
 * Copyright (C) 2018 The Android Open Source Project
 * Copyright (C) 2019 STMicroelectronics
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <filesystem>
#include <iterator>

#include <FileUtils.h>

const std::vector<std::string> FileUtils::listDirectories(const std::string &dirPath)
{
    std::vector<std::string> directoryList;

    for(auto &p: std::filesystem::directory_iterator(dirPath)) {
        if (p.is_directory()) {
            directoryList.push_back(p.path().filename());
        }
    }

    return directoryList;
}

int FileUtils::writeString(const std::string &filename, std::string &value)
{
    std::ofstream mFile;

    mFile.open(filename, std::ofstream::out);
    if (!mFile.is_open()) {
        return -EIO;
    }

    std::ostream &stream = mFile << value;
    if (stream.fail()) {
        return -EIO;
    }

    mFile.close();
    if (mFile.fail()) {
        return -EIO;
    }

    return 0;
}

int FileUtils::readString(const std::string &filename, std::string &value)
{
    std::ifstream mFile;

    mFile.open(filename, std::ifstream::in);
    if (!mFile.is_open()) {
        return -EIO;
    }

    std::istream &stream = mFile >> value;
    if (stream.fail()) {
        return -EIO;
    }

    mFile.close();
    if (mFile.fail()) {
        return -EIO;
    }

    return 0;
}

int FileUtils::writeInt(const std::string &filename, int value)
{
    std::string valueString = std::to_string(value);

    return FileUtils::writeString(filename, valueString);
}

int FileUtils::readInt(const std::string &filename)
{
    std::string value;

    if (auto err = FileUtils::readString(filename, value) < 0) {
        return err;
    }

    return std::stoi(value);
}

int FileUtils::writeFloat(const std::string &filename, float value)
{
    std::string valueString = std::to_string(value);

    return FileUtils::writeString(filename, valueString);
}

int FileUtils::readFloat(const std::string &filename)
{
    std::string value;

    if (auto err = FileUtils::readString(filename, value) < 0) {
        return err;
    }

    return std::stof(value);
}

int FileUtils::readFloatList(const std::string &filename, std::vector<float> &list)
{
    std::string value;

    if (auto err = FileUtils::readString(filename, value) < 0) {
        return err;
    }

    std::istringstream iss(value);
    std::copy(std::istream_iterator<float>(iss),
              std::istream_iterator<float>(),
              std::back_inserter(list));

    return 0;
}
