/**
 *    Copyright (C) 2024-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include <boost/optional/optional.hpp>
#include <string>

#include "mongo/bson/bsonelement.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/bson/json.h"
#include "mongo/db/timeseries/bucket_catalog/measurement_map.h"
#include "mongo/db/timeseries/timeseries_gen.h"
#include "mongo/unittest/death_test.h"
#include "mongo/util/tracking/context.h"

namespace mongo::timeseries::bucket_catalog {
class MeasurementMapTest : public unittest::Test {
public:
    MeasurementMapTest() : measurementMap(trackingContext) {}

protected:
    tracking::Context trackingContext;
    MeasurementMap measurementMap;
};

BSONObj genBucketDoc(int timestampSecondsField) {
    std::string seconds = std::to_string(timestampSecondsField);
    BSONObj bucketDocDataFields = fromjson(
        R"({"data":{"time":{"0":{"$date":"2022-06-06T15:34:" + seconds + ".000Z"},
                            "1":{"$date":"2022-06-06T15:34:" + seconds + ".000Z"},
                            "2":{"$date":"2022-06-06T15:34:" + seconds + ".000Z"}},
                    "a":{"0":1,"1":2,"2":3},
                    "b":{"0":1,"1":2,"2":3}}})");
    return bucketDocDataFields;
}

BSONObj genBucketDoc() {
    BSONObj bucketDocDataFields = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:30.000Z"}},
                    "a":{"0":1},
                    "b":{"0":1}})");
    return bucketDocDataFields;
}

void genMeasurement(std::vector<BSONElement>& elems) {
    BSONObj bucketDocDataFields = genBucketDoc().getOwned();
    for (auto dataField : bucketDocDataFields) {
        elems.push_back(dataField);
    }
}

std::vector<BSONElement> genMeasurementsFromObj(BSONObj obj) {
    std::vector<BSONElement> elems;
    for (auto& dataField : obj) {
        elems.push_back(dataField);
    }
    return elems;
}

std::vector<BSONElement> genMeasurementFieldsFromObj(BSONObj obj) {
    std::vector<BSONElement> elems;
    for (auto& dataField : obj) {
        elems.push_back(dataField);
    }
    return elems;
}

std::vector<BSONElement> genMessyMeasurements() {
    std::vector<BSONElement> elems;
    BSONObj bucketDocDataFields = genBucketDoc();
    for (auto& dataField : bucketDocDataFields) {
        elems.push_back(dataField);
    }
    return elems;
}

TEST_F(MeasurementMapTest, IterationBasic) {
    std::vector<BSONElement> elems;

    // Insert measurement 1.
    BSONObj m1_time = BSON("time" << BSON("0" << BSON("$date" << "2022-06-06T15:34:30.000Z")));
    elems.emplace_back(m1_time.getField("time"));
    BSONObj m1_a = BSON("a" << BSON("0" << "1"));
    elems.emplace_back(m1_a.getField("a"));
    measurementMap.insertOne(elems);

    elems.clear();

    // Insert measurement 2.
    BSONObj m2_time = BSON("time" << BSON("0" << BSON("$date" << "2022-06-06T15:34:31.000Z")));
    elems.emplace_back(m2_time.getField("time"));
    BSONObj m2_a = BSON("a" << BSON("0" << "5"));
    elems.emplace_back(m2_a.getField("a"));
    measurementMap.insertOne(elems);
    invariant(measurementMap.numFields() == 2);
}

TEST_F(MeasurementMapTest, FillSkipsDifferentField) {
    const BSONObj bucketDoc = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:30.000Z"}},
                    "a":{"0":1},
                    "b":{"0":1}})");

    const BSONObj bucketDoc2 = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:31.000Z"}},
                    "a":{"0":1},
                    "b":{"0":1}})");

    const BSONObj bucketDocNewField = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:32.000Z"}},
                    "c":{"4":5}})");

    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDoc));
    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDoc2));
    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDocNewField));
    invariant(measurementMap.numFields() == 4);
}

TEST_F(MeasurementMapTest, FillSkipsAddField) {
    const BSONObj bucketDoc = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:30.000Z"}},
                    "a":{"0":1},
                    "b":{"0":1}})");

    const BSONObj bucketDocWithField = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:35.000Z"}},
                    "a":{"0":4},
                    "b":{"0":1},
                    "c":{"0":1}})");
    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDoc));
    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDocWithField));
    invariant(measurementMap.numFields() == 4);
}

TEST_F(MeasurementMapTest, FillSkipsRemoveField) {
    const BSONObj bucketDoc = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:30.000Z"}},
                    "a":{"0":1},
                    "b":{"0":1}})");

    const BSONObj bucketDocWithoutField = fromjson(
        R"({"time":{"0":{"$date":"2022-06-06T15:34:35.000Z"}},
                    "a":{"0":4}})");
    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDoc));
    measurementMap.insertOne(genMeasurementFieldsFromObj(bucketDocWithoutField));
    invariant(measurementMap.numFields() == 3);
}

TEST_F(MeasurementMapTest, InitBuilders) {
    BSONObj bucketDataDoc;
    BSONObjBuilder bucket;
    BSONObjBuilder dataBuilder = bucket.subobjStart("data");
    BSONColumnBuilder timeColumn;

    BSONObjBuilder bob1;
    bob1.appendTimestamp("$date", 0);
    BSONObjBuilder bob2;
    bob2.appendTimestamp("$date", 1);
    BSONObjBuilder bob3;
    bob3.appendTimestamp("$date", 2);
    timeColumn.append(bob1.done().firstElement());
    timeColumn.append(bob2.done().firstElement());
    timeColumn.append(bob3.done().firstElement());
    BSONBinData timeBinary = timeColumn.finalize();

    BSONColumnBuilder f1Column;
    BSONObj f1m1 = BSON("0" << "1");
    BSONObj f1m2 = BSON("1" << "2");
    BSONObj f1m3 = BSON("2" << "3");
    f1Column.append(f1m1.firstElement());
    f1Column.append(f1m2.firstElement());
    f1Column.append(f1m3.firstElement());
    BSONBinData f1Binary = f1Column.finalize();

    BSONColumnBuilder f2Column;
    BSONObj f2m1 = BSON("0" << "1");
    BSONObj f2m2 = BSON("1" << "1");
    BSONObj f2m3 = BSON("2" << "1");
    f2Column.append(f2m1.firstElement());
    f2Column.append(f2m2.firstElement());
    f2Column.append(f2m3.firstElement());
    BSONBinData f2Binary = f2Column.finalize();

    dataBuilder.append("time", timeBinary);
    dataBuilder.append("a", f1Binary);
    dataBuilder.append("b", f2Binary);

    measurementMap.initBuilders(dataBuilder.done(), 3);
    invariant(measurementMap.numFields() == 3);
}

DEATH_TEST_REGEX_F(MeasurementMapTest, GetTimeForNonexistentField, "Invariant failure.*") {
    measurementMap.timeOfLastMeasurement("time");
}

TEST_F(MeasurementMapTest, ContainsField) {
    std::vector<BSONElement> elems;

    // Insert measurement 1.
    BSONObj m1_time = BSON("time" << BSON("0" << BSON("$date" << "2022-06-06T15:34:30.000Z")));
    elems.emplace_back(m1_time.getField("time"));
    BSONObj m1_a = BSON("a" << BSON("0" << "1"));
    elems.emplace_back(m1_a.getField("a"));
    measurementMap.insertOne(elems);

    elems.clear();

    // Insert measurement 2.
    BSONObj m2_time = BSON("time" << BSON("0" << BSON("$date" << "2022-06-06T15:34:31.000Z")));
    elems.emplace_back(m2_time.getField("time"));
    BSONObj m2_a = BSON("a" << BSON("0" << "5"));
    elems.emplace_back(m2_a.getField("a"));
    measurementMap.insertOne(elems);

    invariant(measurementMap.containsField("a"));
    invariant(!measurementMap.containsField("b"));
}

}  // namespace mongo::timeseries::bucket_catalog
