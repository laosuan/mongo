// Tests whether a reset sharding version triggers errors
import {ShardingTest} from "jstests/libs/shardingtest.js";

var st = new ShardingTest({shards: 1, mongos: 2});

var mongosA = st.s0;
var mongosB = st.s1;

jsTest.log("Adding new collections...");

var collA = mongosA.getCollection(jsTestName() + ".coll");
assert.commandWorked(collA.insert({hello: "world"}));

var collB = mongosB.getCollection("" + collA);
assert.commandWorked(collB.insert({hello: "world"}));

jsTest.log("Enabling sharding...");

assert.commandWorked(mongosA.getDB("admin").adminCommand({enableSharding: "" + collA.getDB()}));
assert.commandWorked(
    mongosA.getDB("admin").adminCommand({shardCollection: "" + collA, key: {_id: 1}}));

// MongoD doesn't know about the config shard version *until* MongoS tells it
collA.findOne();

jsTest.log("Trigger shard version mismatch...");

assert.commandWorked(collB.insert({goodbye: "world"}));

print("Inserted...");

assert.eq(3, collA.find().itcount());
assert.eq(3, collB.find().itcount());

st.stop();
