/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/free_list_impl.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 5 };

struct Object : FreeListNode<> {};

struct RefObject : RefCounted<RefObject, NoopAllocation>, FreeListNode<> {};

} // namespace

TEST_GROUP(free_list) {};

TEST(free_list, empty_list) {
   FreeListImpl list;

   CHECK(list.front() == NULL);

   CHECK(list.is_empty());
}

TEST(free_list, push_front) {
    // push one element
   FreeListData objects[NumObjects];
   FreeListImpl list;

   // FreeListData* element = &objects[0];
   list.push_front(&objects[0]);
// 
//    POINTERS_EQUAL(element, list.front());
// 
//    // LONGS_EQUAL(1, list.size()); 
//    int size = 0;
//    while (list.front() != NULL) {
//    	list.try_pop_front();
//    	size++;
//    }
//    LONGS_EQUAL(1, size);
//    CHECK(!list.is_empty());
}
}
}
//     { // push many elements
//         FreeListNode* objects[NumObjects];
//         FreeListImpl list;
// 
// 	int size = 0;
//         for (size_t i = 0; i < NumObjects; ++i) {
// 
//             list.push_front(objects[i]);
//         }
//         
// 	POINTERS_EQUAL(&objects[NumObjects - 1], list.front());
//         CHECK(!list.is_empty());
// 	
// 	while (list.front() != NULL) {
// 		list.try_pop_front();
// 		size++;
// 	}
// 
//         LONGS_EQUAL(NumObjects, size);
//     }
// }
// 
// TEST(list, try_pop_front) {
//     { // with push_front
//         FreeListNode* objects[NumObjects];
//         FreeListImpl list;
// 	int size = 0;
//         for (size_t i = 0; i < NumObjects; ++i) {
//             list.push_front(objects[i]);
// 	    size++;
//         }
// 
//         for (size_t i = 0; i < NumObjects; ++i) {
//             LONGS_EQUAL(NumObjects - i, size);
//             list.try_pop_front();
// 	    size--;
//             if (i != NumObjects - 1) {
//                 POINTERS_EQUAL(&objects[NumObjects - i - 2], list.front());
//             }
//         }
// 
//         CHECK(list.front() == NULL);
//         LONGS_EQUAL(0, size);
//     }
// }
// 
// // TODO: test a destructor / unsafe_pop_front - which is a private method
// TEST(list, iteration) {
//     FreeListNode* objects[NumObjects];
//     FreeListImpl list;
// 
//     for (size_t i = 0; i < NumObjects; ++i) {
//         list.push_front(objects[i]);
//     }
// 
//     {
//         int i = 0;
//         for (FreeListNode* obj = list.front(); obj != NULL; obj = obj -> next) {
//             POINTERS_EQUAL(&objects[i++], obj);
//         }
//     }
// 
// }
// 
// TEST(list, ownership_operations) {
//     { // push_front
//         RefObject obj;
//         FreeListImpl list;
// 
//         LONGS_EQUAL(0, obj.getref());
//         list.push_front(obj);
//         LONGS_EQUAL(1, obj.getref());
//     }
//     { // pop_front
//         RefObject obj1;
//         RefObject obj2;
// 
//         FreeListImpl list;
// 
//         list.push_front(obj1);
//         list.push_front(obj2);
//         LONGS_EQUAL(1, obj1.getref());
//         LONGS_EQUAL(1, obj2.getref());
// 
//         list.try_pop_front();
//         LONGS_EQUAL(0, obj1.getref());
//         POINTERS_EQUAL(&obj2, list.front().get());
// 
//         list.try_pop_front();
//         LONGS_EQUAL(0, obj2.getref());
//     }
//     { // remove
//         RefObject obj;
//         FreeListImpl list;
// 
//         list.push_front(obj);
//         LONGS_EQUAL(1, obj.getref());
// 
//         list.remove(obj);
//         LONGS_EQUAL(0, obj.getref());
//     }
// }
// 
// TEST(list, ownership_destructor) {
//     RefObject obj;
// 
//     {
//         FreeListImpl list;
// 
//         list.push_front(obj);
// 
//         LONGS_EQUAL(1, obj.getref());
//     }
// 
//     LONGS_EQUAL(0, obj.getref());
// }
// 
// TEST(list, shared_pointers) {
//     RefObject obj;
//     FreeListImpl list;
// 
//     list.push_front(obj);
// 
//     POINTERS_EQUAL(&obj, list.front().get());
// 
//     LONGS_EQUAL(2, list.front()->getref());
// }
 // namespace core

 // namespace roc
