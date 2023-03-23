// ---------------------------------------------------------------------------------
// -- File: gcsr_tests.cpp
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

#if GC_DEBUG_MODE

#include <intrin.h>

#define EPSILON 0.0001
#define TEST_ROW_MAXCHAR_COUNT 80

enum TestMessage
{
    TEST_OK,
    TEST_FAILED,

    TEST_MSG_COUNT
};

char *feedbackMessages[] = {
    "\n  TEST: %s%sOk",
    "\n  TEST: %s%sFailed"
};

size_t feedbackMessagesLength[] = {
    strlen(feedbackMessages[TEST_OK]) - 4,
    strlen(feedbackMessages[TEST_FAILED]) - 4,
};

b32 equalFloat(r32 f1, r32 f2)
{
    b32 res = 0;

    if (fabs(f1 - f2) < EPSILON)
        res = 1;

    return res;
};

void TEST_Start()
{
    printf(SEPARATOR);
    printf("\n--- Starting the tests");
    printf(SEPARATOR "\n");
}

#define TEST_Separator() printf("\n");

void TEST_printResult(TestMessage Msg, char *input)
{
    char *placeholder = feedbackMessages[Msg];
    char buffer[TEST_ROW_MAXCHAR_COUNT];

    s32 length = (s32) (TEST_ROW_MAXCHAR_COUNT - feedbackMessagesLength[Msg] - strlen(input));

    if (length > 0)
    {
        for (s32 i = 0; i < length; ++i) {
            buffer[i] = '.';
        }

        printf(placeholder, input, buffer);
    }
    else
        printf(placeholder, input, "");
}

void TEST_End() {
    printf("\n" SEPARATOR);
}

void TEST_Mat2()
{
    mat2 m = {4, -2, 7, 10};
    mat2 empty = {1, 1, 1, 1};
    mat2_empty(empty);

    mat2 identity;
    mat2_identity(identity);

    // ----------------------------------------------------------------------------------

    if (empty[0][0] == 0 && empty[0][1] == 0 && empty[1][0] == 0 && empty[1][1] == 0)
        TEST_printResult(TEST_OK, "Mat2 empty");
    else
        TEST_printResult(TEST_FAILED, "Mat2 empty");

    // ----------------------------------------------------------------------------------

    if (identity[0][0] == 1 && identity[0][1] == 0 && identity[1][0] == 0 && identity[1][1] == 1)
        TEST_printResult(TEST_OK, "Mat2 identity");
    else
        TEST_printResult(TEST_FAILED, "Mat2 identity");

    // ----------------------------------------------------------------------------------

    r32 expected = 54.0f;
    r32 res = mat2_det(m);

    if (res == expected)
        TEST_printResult(TEST_OK, "Mat2 det");
    else
        TEST_printResult(TEST_FAILED, "Mat2 det");

    // ----------------------------------------------------------------------------------

    mat2 m1 = {1, 2, 3, 4};
    mat2 m2 = {1, 2, 3, 4};
    mat2 out;

    mat2_add(m1, m2, out);

    if (out[0][0] == 2 && out[0][1] == 4 && out[1][0] == 6 && out[1][1] == 8)
        TEST_printResult(TEST_OK, "Mat2 add");
    else
        TEST_printResult(TEST_FAILED, "Mat2 add");

    // ----------------------------------------------------------------------------------

    mat2_sub(m1, m2, out);

    if (out[0][0] == 0 && out[0][1] == 0 && out[1][0] == 0 && out[1][1] == 0)
        TEST_printResult(TEST_OK, "Mat2 sub");
    else
        TEST_printResult(TEST_FAILED, "Mat2 sub");

    // ----------------------------------------------------------------------------------

    mat2_mul(m1, identity, out);

    if (out[0][0] == 1 && out[0][1] == 2 && out[1][0] == 3 && out[1][1] == 4)
        TEST_printResult(TEST_OK, "Mat2 identity mult");
    else
        TEST_printResult(TEST_FAILED, "Mat2 identity mult");

    mat2_mul(m1, m2, out);

    if (out[0][0] == 7 && out[0][1] == 10 && out[1][0] == 15 && out[1][1] == 22)
        TEST_printResult(TEST_OK, "Mat2 mult");
    else
        TEST_printResult(TEST_FAILED, "Mat2 mult");

    // ----------------------------------------------------------------------------------

    mat2_muls(m1, 2, out);

    if (out[0][0] == 2 && out[0][1] == 4 && out[1][0] == 6 && out[1][1] == 8)
        TEST_printResult(TEST_OK, "Mat2 mults");
    else
        TEST_printResult(TEST_FAILED, "Mat2 mults");

    // ----------------------------------------------------------------------------------

    vec2 v = {2, 3};
    vec2 vout = mat2_mulvec(m1, v);

    if (vout.x == 8 && vout.y == 18)
        TEST_printResult(TEST_OK, "Mat2 multvec");
    else
        TEST_printResult(TEST_FAILED, "Mat2 multvec");

    // ----------------------------------------------------------------------------------

    mat2_transpose(m1, out);

    if (out[0][0] == 1 && out[0][1] == 3 && out[1][0] == 2 && out[1][1] == 4)
        TEST_printResult(TEST_OK, "Mat2 transpose");
    else
        TEST_printResult(TEST_FAILED, "Mat2 transpose");

    // ----------------------------------------------------------------------------------

    mat2_inverse(m1, out);

    if (equalFloat(out[0][0], -2) &&
        equalFloat(out[0][1], 1) &&
        equalFloat(out[1][0], 3.0f / 2.0f) &&
        equalFloat(out[1][1], -1.0f / 2.0f))
        TEST_printResult(TEST_OK, "Mat2 inverse");
    else
        TEST_printResult(TEST_FAILED, "Mat2 inverse");
}

void TEST_Mat3()
{
    mat3 m = {
        10, 2, 3,
        4, 5, 6,
        7, 8, 9
    };
    mat3 empty = {1, 1, 1, 1, 1, 1, 1, 1, 1};
    mat3_empty(empty);

    if (empty[0][0] == 0 && empty[1][1] == 0 && empty[2][2] == 0)
        TEST_printResult(TEST_OK, "Mat3 empty");
    else
        TEST_printResult(TEST_FAILED, "Mat3 empty");

    // ----------------------------------------------------------------------------------

    mat3_identity(empty);

    if (empty[0][0] == 1 && empty[1][1] == 1 && empty[2][2] == 1)
        TEST_printResult(TEST_OK, "Mat3 identity");
    else
        TEST_printResult(TEST_FAILED, "Mat3 identity");

    // ----------------------------------------------------------------------------------

    mat3 m1 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    mat3 m2 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    mat3 m3 = {2, 4, 6, 8, 10, 12, 14, 16, 18};
    mat3 expected;

    mat3_copy(expected, m3);

    mat3 out;
    mat3_add(m1, m2, out);

    if (mat3_equals(out, expected))
        TEST_printResult(TEST_OK, "Mat3 add");
    else
        TEST_printResult(TEST_FAILED, "Mat3 add");

    // ----------------------------------------------------------------------------------

    mat3_empty(expected);
    mat3_sub(m1, m2, out);

    if (mat3_equals(out, expected))
        TEST_printResult(TEST_OK, "Mat3 sub");
    else
        TEST_printResult(TEST_FAILED, "Mat3 sub");

    // ----------------------------------------------------------------------------------

    mat3 mat = {
        4, -2, 1,
        7, 10, -3,
        1, 0, 10
    };
    r32 expected_det = 536.0f;
    r32 res = mat3_det(mat);

    if (res == expected_det)
        TEST_printResult(TEST_OK, "Mat3 det");
    else
        TEST_printResult(TEST_FAILED, "Mat3 det");

    // ----------------------------------------------------------------------------------

    mat3_copy(expected, m3);
    mat3_muls(m1, 2, out);

    if (mat3_equals(out, expected))
        TEST_printResult(TEST_OK, "Mat3 mults");
    else
        TEST_printResult(TEST_FAILED, "Mat3 mults");

    // ----------------------------------------------------------------------------------

    vec3 v = {1, 2, 3};
    vec3 res_vec3 = mat3_mulvec(m1, v);
    vec3 expected_v3 = {14, 32, 50};

    if (vec3_equals(expected_v3, res_vec3))
        TEST_printResult(TEST_OK, "Mat3 multvec");
    else
        TEST_printResult(TEST_FAILED, "Mat3 multvec");

    // ----------------------------------------------------------------------------------

    mat3 expected2 = {30, 36, 42, 66, 81, 96, 102, 126, 150};
    mat3_mul(m1, m2, out);

    if (mat3_equals(expected2, out))
        TEST_printResult(TEST_OK, "Mat3 mult");
    else
        TEST_printResult(TEST_FAILED, "Mat3 mult");

    // ----------------------------------------------------------------------------------

    mat3_transpose(m1, out);
    mat3 expected3 = {1, 4, 7, 2, 5, 8, 3, 6, 9};

    if (mat3_equals(expected3, out))
        TEST_printResult(TEST_OK, "Mat3 transpose");
    else
        TEST_printResult(TEST_FAILED, "Mat3 transpose");

    // ----------------------------------------------------------------------------------

    mat3 inv = {1, 2, 3, 0, 1, 4, 2, 1, 5};
    mat3 expected4 = {1.0f/11, -7.0f/11, 5.0f/11, 8.0f/11, -1.0f/11, -4.0f/11, -2.0f/11, 3.0f/11, 1.0f/11};

    mat3_inv(inv, out);

    if (mat3_equals(expected4, out))
        TEST_printResult(TEST_OK, "Mat3 inverse");
    else
        TEST_printResult(TEST_FAILED, "Mat3 inverse");

    mat3 out2;
    mat3_mul(out, inv, out2);

    if (equalFloat(out2[0][0], 1) &&
        equalFloat(out2[0][1], 0) &&
        equalFloat(out2[0][2], 0) &&
        equalFloat(out2[1][0], 0) &&
        equalFloat(out2[1][1], 1) &&
        equalFloat(out2[1][2], 0) &&
        equalFloat(out2[2][0], 0) &&
        equalFloat(out2[2][1], 0) &&
        equalFloat(out2[2][2], 1))

        TEST_printResult(TEST_OK, "Mat3 inverse mult identity");
    else
        TEST_printResult(TEST_FAILED, "Mat3 inverse mult identity");
}

void TEST_Mat4()
{
    mat4 out;
    mat4 expected_empty = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    mat4 expected_identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    mat4_empty(out);

    if (mat4_equals(expected_empty, out))
        TEST_printResult(TEST_OK, "Mat4 empty");
    else
        TEST_printResult(TEST_FAILED, "Mat4 empty");

    // ----------------------------------------------------------------------------------

    mat4_identity(out);

    if (mat4_equals(expected_identity, out))
        TEST_printResult(TEST_OK, "Mat4 identity");
    else
        TEST_printResult(TEST_FAILED, "Mat4 identity");

    // ----------------------------------------------------------------------------------

    mat4 mat = {
        4, -2, 1, 6,
        7, 10, -3, 2,
        1, 0, 10, -4,
        2, 4, 5, 1
    };
    r32 expected = 2368.0f;
    r32 res = mat4_det(mat);

    if (res == expected)
        TEST_printResult(TEST_OK, "Mat4 det");
    else
        TEST_printResult(TEST_FAILED, "Mat4 det");

    // ----------------------------------------------------------------------------------

    mat4 m1 = {
        1, 2, 3, 4,
        1, 2, 3, 4,
        1, 2, 3, 4,
        1, 2, 3, 4
    };
    mat4 expected_mat = {
        2, 4, 6, 8,
        2, 4, 6, 8,
        2, 4, 6, 8,
        2, 4, 6, 8
    };

    mat4_add(m1, m1, out);

    if (mat4_equals(expected_mat, out))
        TEST_printResult(TEST_OK, "Mat4 add");
    else
        TEST_printResult(TEST_FAILED, "Mat4 add");

    // ----------------------------------------------------------------------------------

    mat4_sub(m1, m1, out);

    if (mat4_equals(expected_empty, out))
        TEST_printResult(TEST_OK, "Mat4 sub");
    else
        TEST_printResult(TEST_FAILED, "Mat4 sub");

    // ----------------------------------------------------------------------------------

    mat4 expected_mult = {
        10, 20, 30, 40,
        10, 20, 30, 40,
        10, 20, 30, 40,
        10, 20, 30, 40
    };

    mat4_mul(m1, m1, out);

    if (mat4_equals(out, expected_mult))
        TEST_printResult(TEST_OK, "Mat4 mult");
    else
        TEST_printResult(TEST_FAILED, "Mat4 mult");

    // ----------------------------------------------------------------------------------

    mat4_muls(m1, 2, out);

    if (mat4_equals(out, expected_mat))
        TEST_printResult(TEST_OK, "Mat4 mults");
    else
        TEST_printResult(TEST_FAILED, "Mat4 mults");

    // ----------------------------------------------------------------------------------

    vec4 v = {1, 2, 3, 4};
    vec4 vec_out = mat4_mulvec(expected_identity, v);

    if (vec4_equals(vec_out, v))
        TEST_printResult(TEST_OK, "Mat4 multvec");
    else
        TEST_printResult(TEST_FAILED, "Mat4 multvec");

    // ----------------------------------------------------------------------------------

    mat4 expected_transpose = {
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3, 3, 3,
        4, 4, 4, 4
    };

    mat4_transpose(m1, out);

    if (mat4_equals(out, expected_transpose))
        TEST_printResult(TEST_OK, "Mat4 transpose");
    else
        TEST_printResult(TEST_FAILED, "Mat4 transpose");

    // ----------------------------------------------------------------------------------

    mat4 m2 = {
        1, 0, 0, 100,
        0, 0.8f, -1, 200,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    mat4 mat_inv;
    // mat4 expected_inv;

    mat4_inv(m2, mat_inv);
    mat4_mul(mat_inv, m2, out);

    if (equalFloat(out[0][0], 1) &&
        equalFloat(out[0][1], 0) &&
        equalFloat(out[0][2], 0) &&
        equalFloat(out[0][3], 0) &&
        equalFloat(out[1][0], 0) &&
        equalFloat(out[1][1], 1) &&
        equalFloat(out[1][2], 0) &&
        equalFloat(out[1][3], 0) &&
        equalFloat(out[2][0], 0) &&
        equalFloat(out[2][1], 0) &&
        equalFloat(out[2][2], 1) &&
        equalFloat(out[2][3], 0) &&
        equalFloat(out[3][0], 0) &&
        equalFloat(out[3][1], 0) &&
        equalFloat(out[3][2], 0) &&
        equalFloat(out[3][3], 1))

        TEST_printResult(TEST_OK, "Mat4 inverse mult identity");
    else
        TEST_printResult(TEST_FAILED, "Mat4 inverse mult identity");
}

void TEST_Vec2()
{
    vec2 v1 = {1, 2};
    vec2 v2 = {3, 4};

    vec2 res = vec2_add(v1, v2);

    if (res.x == 4 && res.y == 6)
        TEST_printResult(TEST_OK, "Vec2 add");
    else
        TEST_printResult(TEST_FAILED, "Vec2 add");

    // ----------------------------------------------------------------------------------

    res = vec2_sub(v1, v2);

    if (res.x == -2 && res.y == -2)
        TEST_printResult(TEST_OK, "Vec2 sub");
    else
        TEST_printResult(TEST_FAILED, "Vec2 sub");

    // ----------------------------------------------------------------------------------

    res = vec2_muls(v1, 2);

    if (res.x == 2 && res.y == 4)
        TEST_printResult(TEST_OK, "Vec2 mults");
    else
        TEST_printResult(TEST_FAILED, "Vec2 mults");

    // ----------------------------------------------------------------------------------

    res = vec2_inv(v1);

    if (res.x == -1 && res.y == -2)
        TEST_printResult(TEST_OK, "Vec2 inv");
    else
        TEST_printResult(TEST_FAILED, "Vec2 inv");

    // ----------------------------------------------------------------------------------

    r32 dot = vec2_dot(v1, v1);

    if (dot == 5)
        TEST_printResult(TEST_OK, "Vec2 dot");
    else
        TEST_printResult(TEST_FAILED, "Vec2 dot");

    // ----------------------------------------------------------------------------------

    r32 len = vec2_len(v1);

    if (equalFloat(len, 2.23606797749978969641f))
        TEST_printResult(TEST_OK, "Vec2 len");
    else
        TEST_printResult(TEST_FAILED, "Vec2 len");
}


void TEST_LinkedListU32()
{
    Type_LinkedList *U32List = tll_create(MEMORY_TEMPORARY, gcSize(u32));

    u32 *v = (u32 *) tll_insert(U32List);
    *v = 100;

    v = (u32 *) tll_insert(U32List);
    *v = 200;

    v = (u32 *) tll_insert(U32List);
    *v = 300;

    v = (u32 *) tll_insert(U32List);
    *v = 300;

    u32 *value = (u32 *) tll_get(U32List, 1);

    // TLL_Print(U32List);
}

void TEST_LinkedListPerson()
{
    Type_LinkedList *PersonList = tll_create(MEMORY_TEMPORARY, gcSize(Person));
    DBG_SetDataType(PersonList, DATA_PERSON);
    Person *Dude = 0, *Search = 0;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Johnson");
    strcpy_s(Dude->surname, 40, "Mick");
    Dude->age = 30;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Blaster");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 100;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person0");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 20;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person1");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 30;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person2");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 31;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person3");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 32;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person4");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 33;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person5");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 34;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person6");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 35;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person7");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 36;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person8");
    strcpy_s(Dude->surname, 40, "Master");
    Dude->age = 37;

    // ---------------------------------------------------------------------------------
    // -- Simple search.
    // ---------------------------------------------------------------------------------

    Search = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person7");

    if (Search != 0 && strcmp(Search->name, "Person7") == 0)
        TEST_printResult(TEST_OK, "List node search");
    else
        TEST_printResult(TEST_FAILED, "List node search");

    // ---------------------------------------------------------------------------------
    // -- Node delete.
    // ---------------------------------------------------------------------------------

    tll_delete(PersonList, Search);

    Search = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person7");

    if (Search == 0)
        TEST_printResult(TEST_OK, "List node delete");
    else
        TEST_printResult(TEST_FAILED, "List node delete");

    // ---------------------------------------------------------------------------------
    // -- Adjacent node switch.
    // ---------------------------------------------------------------------------------

    Person *Search1, *Search2;

    Search1 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person1");
    Search2 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person2");

    tll_switchNodes(PersonList, Search1, Search2);

    Search1 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person0");
    Type_LinkedListNode *SearchNode1 = TLL_GetNode(Search1);
    Search1 = TLL_GetData(SearchNode1->Next, Person);

    if (strcmp(Search1->name, "Person2") == 0)
        TEST_printResult(TEST_OK, "List node switch");
    else
        TEST_printResult(TEST_FAILED, "List node switch");

    // ---------------------------------------------------------------------------------
    // -- Distant node switch.
    // ---------------------------------------------------------------------------------

    Search1 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person1");
    Search2 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person5");

    tll_switchNodes(PersonList, Search1, Search2);

    Search1 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person4");

    SearchNode1 = TLL_GetNode(Search1);
    Search1 = TLL_GetData(SearchNode1->Next, Person);

    if (strcmp(Search1->name, "Person1") == 0)
        TEST_printResult(TEST_OK, "List node switch2");
    else
        TEST_printResult(TEST_FAILED, "List node switch2");

    // ---------------------------------------------------------------------------------
    // -- Deleting a node from the end of the list.
    // ---------------------------------------------------------------------------------

    u32 used_old = PersonList->length;

    Person *TmpPerson = TLL_GetLast(PersonList, Person);
    tll_delete(PersonList, TmpPerson);

    if (PersonList->length == used_old - 1)
        TEST_printResult(TEST_OK, "List node delete tail check 1");
    else
        TEST_printResult(TEST_FAILED, "List node delete tail check 1");

    Search1 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person10");

    if (!Search1)
        TEST_printResult(TEST_OK, "List node delete tail check 2");
    else
        TEST_printResult(TEST_FAILED, "List node delete tail check 2");

    // ---------------------------------------------------------------------------------
    // -- Deleting a node from the head of the list.
    // ---------------------------------------------------------------------------------

    used_old = PersonList->length;

    TmpPerson = TLL_GetFirst(PersonList, Person);
    tll_delete(PersonList, TmpPerson);

    if (PersonList->length == used_old - 1)
        TEST_printResult(TEST_OK, "List node delete head check 1");
    else
        TEST_printResult(TEST_FAILED, "List node delete head check 1");

    Search1 = (Person *) tll_search(PersonList, tll_searchPersonByName, "Johnson");

    if (!Search1)
        TEST_printResult(TEST_OK, "List node delete head check 2");
    else
        TEST_printResult(TEST_FAILED, "List node delete head check 2");
}


void TEST_LinkedListPerson2()
{
    Type_LinkedList *PersonList = tll_create(MEMORY_TEMPORARY, gcSize(Person));
    Person *Dude = 0, *Search = 0;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Johnson");
    strcpy_s(Dude->surname, 40, "Mick");
    Dude->age = 30;

    u32 used_old = PersonList->length;

    tll_delete(PersonList, Dude);

    if (PersonList->length == used_old - 1)
        TEST_printResult(TEST_OK, "List empty check");
    else
        TEST_printResult(TEST_FAILED, "List empty check");
}

void TEST_LinkedListClear()
{
    Type_LinkedList *PersonList = tll_create(MEMORY_TEMPORARY, gcSize(Person));
    Person *Dude = 0, *Search = 0;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 1");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 2");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 3");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 4");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 5");

    // ---------------------------------------------------------------------------------

    Search = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person 1");
    Dude = (Person *) tll_insertPre(PersonList, Search);
    strcpy_s(Dude->name, 40, "Person 6");

    Dude = TLL_GetPrev(Search, Person);

    if (Dude != 0 && strcmp(Dude->name, "Person 6") == 0)
        TEST_printResult(TEST_OK, "List TLL_GetPrev");
    else
        TEST_printResult(TEST_FAILED, "List TLL_GetPrev");

    // ---------------------------------------------------------------------------------

    Search = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person 5");
    Dude = (Person *) tll_insertPost(PersonList, Search);
    strcpy_s(Dude->name, 40, "Person 7");

    Dude = TLL_GetNext(Search, Person);

    if (Dude != 0 && strcmp(Dude->name, "Person 7") == 0)
        TEST_printResult(TEST_OK, "List TLL_GetNext 1");
    else
        TEST_printResult(TEST_FAILED, "List TLL_GetNext 1");

    // ---------------------------------------------------------------------------------

    Dude = TLL_GetNext(Dude, Person);

    if (Dude == 0)
        TEST_printResult(TEST_OK, "List TLL_GetNext 2");
    else
        TEST_printResult(TEST_FAILED, "List TLL_GetNext 2");

    // ---------------------------------------------------------------------------------

    tll_clear(PersonList);

    if (PersonList->length == 0)
        TEST_printResult(TEST_OK, "List tll_clear");
    else
        TEST_printResult(TEST_FAILED, "List tll_clear");
}

void TEST_ArrayU32()
{
    Type_Array *DataArray = ta_create(MEMORY_TEMPORARY, 10, gcSize(u32));

    if (!DataArray)
        TEST_printResult(TEST_FAILED, "Array allocation");
    else
    {
        TEST_printResult(TEST_OK, "Array allocation");

        u32 *value = TA_Index(DataArray, 0, u32);
        *value = 10;

        value = TA_Index(DataArray, 1, u32);
        *value = 15;

        value = TA_Index(DataArray, 2, u32);
        *value = 20;

        value = TA_Index(DataArray, 0, u32);

        if (*value == 10)
            TEST_printResult(TEST_OK, "Array insert at index");
        else
            TEST_printResult(TEST_FAILED, "Array insert at index");

        // ----------------------------------------------------------------------------------

        value = TA_Index(DataArray, 1, u32);

        if (*value == 15)
            TEST_printResult(TEST_OK, "Array check index");
        else
            TEST_printResult(TEST_FAILED, "Array check index");

        // ----------------------------------------------------------------------------------

        ta_delete(DataArray, 1);

        value = TA_Index(DataArray, 1, u32);

        if (*value == 0)
            TEST_printResult(TEST_OK, "Array delete index");
        else
            TEST_printResult(TEST_FAILED, "Array delete index");
    }
}

void TEST_ArrayU32Clear()
{
    Type_Array *DataArray = ta_create(MEMORY_TEMPORARY, 10, gcSize(u32));

    u32 *value = TA_Index(DataArray, 0, u32);
    *value = 10;

    value = TA_Index(DataArray, 2, u32);
    *value = 20;

    ta_clear(DataArray);

    value = TA_Index(DataArray, 0, u32);

    if (*value == 0)
        TEST_printResult(TEST_OK, "Array clear");
    else
        TEST_printResult(TEST_FAILED, "Array clear");
}

void TEST_FreeMemoryBlock()
{
    memory_chunk_t *Chunk = GET_CHUNK(MEMORY_TEMPORARY);
    void *data = gc_mem_allocate(MEMORY_TEMPORARY, Kilobytes(1));

    u32 preFilledBlocks = Chunk->filled_blocks;

    gc_mem_free(data);

    if (Chunk->filled_blocks == preFilledBlocks - 1)
        TEST_printResult(TEST_OK, "memory free");
    else
        TEST_printResult(TEST_FAILED, "memory free");
}

void TEST_LinkedListDelete()
{
    memory_chunk_t *Chunk = GET_CHUNK(MEMORY_TEMPORARY);

    Type_LinkedList *PersonList = tll_create(MEMORY_TEMPORARY, gcSize(Person));
    Person *Dude = 0, *Delete = 0;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 1");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 2");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 3");
    Delete = Dude;

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 4");

    Dude = (Person *) tll_insert(PersonList);
    strcpy_s(Dude->name, 40, "Person 5");

    tll_delete(PersonList, Delete);

    if (PersonList->length == 4)
        TEST_printResult(TEST_OK, "List delete node");
    else
        TEST_printResult(TEST_FAILED, "List delete node");

    // ----------------------------------------------------------------------------------

    Delete = (Person *) tll_search(PersonList, tll_searchPersonByName, "Person 3");

    if (Delete == 0)
        TEST_printResult(TEST_OK, "List delete node search");
    else
        TEST_printResult(TEST_FAILED, "List delete node search");
}

void TEST_ClearMemory()
{
    memory_chunk_t *Chunk = GET_CHUNK(MEMORY_TEMPORARY);

    gc_mem_clear();

    if (Chunk->allocated_bytes == 0 && Chunk->empty_blocks == 1 && Chunk->filled_blocks == 0 && Chunk->block_count == 1)
        TEST_printResult(TEST_OK, "memory clear");
    else
        TEST_printResult(TEST_FAILED, "memory clear");
}


void TEST_MemoryOpsCheck()
{
    memory_chunk_t *Chunk = GET_CHUNK(MEMORY_TEMPORARY);

    Person *Dude = (Person *) gc_mem_allocate(MEMORY_TEMPORARY, gcSize(Person));
    u32 result = 0;

    result = Chunk->empty_blocks == 1;

    Type_LinkedList *PersonList = tll_create(MEMORY_TEMPORARY, gcSize(Person));
    Type_Array *PersonArray = ta_create(MEMORY_TEMPORARY, 50, gcSize(Person));
    Person *Dude2 = (Person *) gc_mem_allocate(MEMORY_TEMPORARY, gcSize(Person));
    DBG_SetDataType(GET_BLOCK(Dude2), DATA_PERSON);

    gc_mem_free(Dude);
    result = result && Chunk->empty_blocks == 2;

    gc_mem_free(PersonArray);
    result = result && Chunk->empty_blocks == 3;

    gc_mem_free(PersonList);
    result = result && Chunk->empty_blocks == 2;

    if (result)
        TEST_printResult(TEST_OK, "memory free check");
    else
        TEST_printResult(TEST_FAILED, "memory free check");
}

void TEST_MemoryAllocateStackArray()
{
    Type_StackArray *Stack = tsa_create(MEMORY_TEMPORARY, 5, gcSize(Person));
    DBG_SetDataType(Stack, DATA_PERSON);

    Person *Dude = (Person *) tsa_push(Stack);
    strcpy_s(Dude->name, 40, "Person 1");
    Dude->age = 1;

    Dude = (Person *) tsa_push(Stack);
    strcpy_s(Dude->name, 40, "Person 2");
    Dude->age = 2;

    Dude = (Person *) tsa_push(Stack);
    strcpy_s(Dude->name, 40, "Person 3");
    Dude->age = 3;

    if (Stack->top + 1 == 3)
        TEST_printResult(TEST_OK, "Stack allocation");
    else
        TEST_printResult(TEST_FAILED, "Stack allocation");

    tsa_pop(Stack);
    Dude = (Person *) tsa_peek(Stack);

    if (strcmp(Dude->name, "Person 2") == 0)
        TEST_printResult(TEST_OK, "Stack pop");
    else
        TEST_printResult(TEST_FAILED, "Stack pop");

    Dude = (Person *) tsa_push(Stack);
    // strcpy_s(Dude->name, 40, "Person 3");
    // Dude->age = 3;

    if (Stack->top + 1 == 3)
        TEST_printResult(TEST_OK, "Stack allocation");
    else
        TEST_printResult(TEST_FAILED, "Stack allocation");

    memory_chunk_t *Chunk = GET_CHUNK(MEMORY_TEMPORARY);
    u32 preBlockCount = Chunk->block_count;

    Type_StackArray *NewStack = tsa_create(MEMORY_TEMPORARY, 10, gcSize(Person));

    if (Chunk->block_count == preBlockCount + 1)
        TEST_printResult(TEST_OK, "New stack allocation");
    else
        TEST_printResult(TEST_FAILED, "New stack allocation");

    tsa_free(NewStack);
}

void TEST_MemoryAllocateQueue()
{
    Type_Queue *Queue = tq_create(MEMORY_TEMPORARY, gcSize(Person));
    DBG_SetDataType(Queue, DATA_PERSON);

    if (Queue)
        TEST_printResult(TEST_OK, "New queue allocation");
    else
        TEST_printResult(TEST_FAILED, "New queue allocation");
}

void TEST_QueueOperations()
{
    Type_Queue *CarQueue = tq_create(MEMORY_TEMPORARY, gcSize(Car));
    DBG_SetDataType(CarQueue, DATA_CAR);

    Car *C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 1");
    C->wheelCount = 4;
    C->doorCount = 4;

    C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 2");
    C->wheelCount = 3;
    C->doorCount = 2;

    C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 3");
    C->wheelCount = 2;
    C->doorCount = 1;

    if (CarQueue->length == 3)
        TEST_printResult(TEST_OK, "Queue length check");
    else
        TEST_printResult(TEST_FAILED, "Queue length check");

    tq_dequeue(CarQueue);
    C = (Car *) tq_front(CarQueue);

    if (strcmp(C->name, "Car 2") == 0)
        TEST_printResult(TEST_OK, "Queue dequeue check");
    else
        TEST_printResult(TEST_FAILED, "Queue dequeue check");

    C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 4");
    C->wheelCount = 2;
    C->doorCount = 1;

    C = (Car *) tq_rear(CarQueue);

    if (strcmp(C->name, "Car 4") == 0)
        TEST_printResult(TEST_OK, "Queue enqueue check");
    else
        TEST_printResult(TEST_FAILED, "Queue enqueue check");
}

void TEST_QueueClear()
{
    Type_Queue *CarQueue = tq_create(MEMORY_TEMPORARY, gcSize(Car));
    DBG_SetDataType(CarQueue, DATA_CAR);

    Car *C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 1");
    C->wheelCount = 4;
    C->doorCount = 4;

    C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 2");
    C->wheelCount = 3;
    C->doorCount = 2;

    C = (Car *) tq_enqueue(CarQueue);
    strcpy_s(C->name, 40, "Car 3");
    C->wheelCount = 2;
    C->doorCount = 1;

    tq_clear(CarQueue);

    if (CarQueue->length == 0)
        TEST_printResult(TEST_OK, "Queue clear");
    else
        TEST_printResult(TEST_FAILED, "Queue clear");

    tq_delete(CarQueue);
}

// ----------------------------------------------------------------------------------

#define NPAD 10
#define ELEMENTS 40

struct cache_check_a
{
    cache_check_a *n;
    u64 pad[NPAD];
};

void TEST_CPUCache()
{
    u32 ws_elements = ELEMENTS;
    u32 ws = ws_elements * sizeof(cache_check_a);

    cache_check_a *List = (cache_check_a *) malloc(ws);
    cache_check_a *Current = List + 1;
    cache_check_a *Prev = 0;

    for (u32 i = 1; i < ws_elements; ++i)
    {
        Prev = Current - 1;
        Prev->n = Current;
        Current->n = 0;

        Current++;
    }

    printf("\n" SEPARATOR);
    printf("\n  CPU cache tests");
    printf(SEPARATOR);
    printf("\n  working set bytes: %u bytes", ws);
    printf(SEPARATOR);

    Current = List;

    u64 cycles[ELEMENTS];
    u32 index = 0;
    u64 total_start = __rdtsc();

    for (u32 i = 0; i < ELEMENTS; ++i)
    {
        u32 start_aux;
        u64 start = __rdtscp(&start_aux);

        Current->pad[0]++;
        // for (u32 j = 0; j < NPAD; ++j)
        //     Current->pad[j]++;

        Current++;

        u32 end_aux;
        u64 end = __rdtscp(&end_aux);

        cycles[i] = end - start;
    }

    u64 total_end = __rdtsc();

    for (u32 i = 0; i < ws_elements; ++i) {
        printf("\n cycles/element: %llu", cycles[i]);
    }

    printf("\n Total cycles: %llu", total_end - total_start);
}

void TEST_Hashtable()
{
    // Type_Hashtable *Table = memAllocateHashtable(MEMORY_TEMPORARY, 10);
    Type_Hashtable *Table = tht_create(MEMORY_TEMPORARY, 10);
    DBG_SetDataType(Table, DATA_CAR);

    // ----------------------------------------------------------------------------------

    Car *C = (Car *) tht_insert(Table, "key1", gcSize(Car));
    strcpy_s(C->name, 40, "Car 1");
    C->wheelCount = 4;
    C->doorCount = 4;

    Type_HashtableElement *Search = tht_search(Table, "key1");

    if (Search)
        TEST_printResult(TEST_OK, "Hashtable search key");
    else
        TEST_printResult(TEST_FAILED, "Hashtable search key");

    // ----------------------------------------------------------------------------------

    C = (Car *) tht_insert(Table, "key1", gcSize(Car));
    strcpy_s(C->name, 40, "Car 1a");
    C->wheelCount = 2;
    C->doorCount = 4;

    if (Table->used == 1)
        TEST_printResult(TEST_OK, "Hashtable overwrite key");
    else
        TEST_printResult(TEST_FAILED, "Hashtable overwrite key");

    // ----------------------------------------------------------------------------------

    Search = tht_search(Table, "key2");

    if (!Search)
        TEST_printResult(TEST_OK, "Hashtable search non existing key");
    else
        TEST_printResult(TEST_FAILED, "Hashtable search non existing key");

    // ----------------------------------------------------------------------------------

    C = (Car *) tht_insert(Table, "key2", gcSize(Car));
    strcpy_s(C->name, 40, "Car 2");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key3", gcSize(Car));
    strcpy_s(C->name, 40, "Car 3");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key4", gcSize(Car));
    strcpy_s(C->name, 40, "Car 4");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key5", gcSize(Car));
    strcpy_s(C->name, 40, "Car 5");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key6", gcSize(Car));
    strcpy_s(C->name, 40, "Car 6");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key7", gcSize(Car));
    strcpy_s(C->name, 40, "Car 7");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key8", gcSize(Car));
    strcpy_s(C->name, 40, "Car 8");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key9", gcSize(Car));
    strcpy_s(C->name, 40, "Car 9");
    C->wheelCount = 4;
    C->doorCount = 1;

    C = (Car *) tht_insert(Table, "key10", gcSize(Car));
    strcpy_s(C->name, 40, "Car 10");
    C->wheelCount = 4;
    C->doorCount = 1;

    if (Table->used == 10)
        TEST_printResult(TEST_OK, "Hashtable count");
    else
        TEST_printResult(TEST_FAILED, "Hashtable count");

    // ----------------------------------------------------------------------------------

    tht_delete(Table, "key1");
    Search = tht_search(Table, "key1");

    if (Table->used == 9 && !Search)
        TEST_printResult(TEST_OK, "Hashtable delete");
    else
        TEST_printResult(TEST_FAILED, "Hashtable delete");

    tht_hashtable_free(Table);
}

void TEST_Strings()
{
    Type_String *String1 = ts_create(MEMORY_TEMPORARY, "Gabi testeaza string-uri");
    Type_String *String2 = ts_create(MEMORY_TEMPORARY, "si nu iese nimic !");

    size_t len1 = ts_length(String1);
    size_t len2 = ts_length(String2);

    if (len1 == 24 && len2 == 18)
        TEST_printResult(TEST_OK, "String length");
    else
        TEST_printResult(TEST_FAILED, "String length");

    // ----------------------------------------------------------------------------------

    ts_append(String1, String2);

    Type_String *String3 = ts_getAppended(String1);
    size_t len3 = ts_length(String3);

    if (len3 == 42)
        TEST_printResult(TEST_OK, "String append length");
    else
        TEST_printResult(TEST_FAILED, "String append length");

    // ----------------------------------------------------------------------------------

    Type_String *EmptyString = ts_createEmpty(MEMORY_TEMPORARY, 10);
    ts_copy(EmptyString, "Un alt test de string-uri.");

    size_t len4 = ts_length(EmptyString);

    if (len4 == 10)
        TEST_printResult(TEST_OK, "String copy length");
    else
        TEST_printResult(TEST_FAILED, "String copy length");

    gc_mem_free(String1);
}

void TEST_MemoryMarkers()
{
    memory_chunk_t *Chunk = GET_CHUNK(MEMORY_TEMPORARY);
    u32 filled_blocks = Chunk->filled_blocks;

    mem_markStart();

    void *Test1 = gc_mem_allocate(MEMORY_TEMPORARY, 64);
    void *Test2 = gc_mem_allocate(MEMORY_TEMPORARY, 128);
    void *Test3 = gc_mem_allocate(MEMORY_TEMPORARY, 256);
    void *Test4 = gc_mem_allocate(MEMORY_TEMPORARY, 512);

    mem_markStop();

    if (filled_blocks == Chunk->filled_blocks)
        TEST_printResult(TEST_OK, "memory markers check");
    else
        TEST_printResult(TEST_FAILED, "memory markers check");
}

#endif