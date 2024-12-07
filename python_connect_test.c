#define PY_SSIZE_T_CLEAN
#include <Python.h>

void initialize_python() {
    Py_Initialize();
    PyObject *sys_path = PySys_GetObject("path");
    PyList_Append(sys_path, PyUnicode_DecodeFSDefault("."));
}

void finalize_python() {
    Py_Finalize();
}

int call_python_function(const char *python_name, const char *python_function, const char *parameter1, const char *parameter2) {
    // Python 모듈 로드
    PyObject *pModule = PyImport_ImportModule(python_name);
    if (pModule == NULL) {
        PyErr_Print();
        return -1;  // 모듈 로드 실패
    }

    // Python 함수 가져오기
    PyObject *pFunc = PyObject_GetAttrString(pModule, python_function);
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        return -1;  // 함수 가져오기 실패
    }

    // 파라미터 처리
    PyObject *pArgs;
    if (parameter1 != NULL && parameter2 == NULL) {
        pArgs = PyTuple_Pack(1, PyUnicode_DecodeFSDefault(parameter1));
    } else if (parameter1 != NULL && parameter2 != NULL) {
        pArgs = PyTuple_Pack(2, PyUnicode_DecodeFSDefault(parameter1), PyUnicode_DecodeFSDefault(parameter2));
    } else {
        pArgs = PyTuple_New(0);
    }

    if (pArgs == NULL) {
        PyErr_Print();
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return -1;  // 인자 패킹 실패
    }

    // 함수 호출
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    if (pValue == NULL) {
        PyErr_Print();
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return -1;  // 함수 호출 실패
    }

    // 반환값을 C의 int로 변환
    int result = (int)PyLong_AsLong(pValue);
    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_DECREF(pValue);
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return -1;  // 변환 오류
    }

    // 메모리 해제
    Py_DECREF(pValue);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    return result;  // 반환값
}

int main() {
    // Python 인터프리터 초기화
    initialize_python();

    // RFID 문자열
    
    char *rfid = (char *)malloc(13 * sizeof(char));
    rfid = "027812690245";


    // Python 함수 호출
    int result = call_python_function("lcd", "lcd_execute", "ao", "bo");
    int valid = call_python_function("database", "find_face_path", rfid, NULL);

    printf("%d\n", result);
    printf("%d\n", valid);

    // Python 인터프리터 종료
    finalize_python();

    return 0;
}
