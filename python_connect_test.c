#include <Python.h>

int main() {
    Py_Initialize();

    // sys.path에 현재 디렉터리 추가
    PyObject *sys_path = PySys_GetObject("path");
    PyList_Append(sys_path, PyUnicode_DecodeFSDefault("."));

    PyObject *pName = PyUnicode_DecodeFSDefault("database");
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        PyObject *pFunc = PyObject_GetAttrString(pModule, "make_db");
        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                printf("make_db() 실행 성공\n");
                Py_DECREF(pValue);
            } else {
                PyErr_Print();
                fprintf(stderr, "make_db() 실행 중 오류 발생\n");
            }
        } else {
            if (PyErr_Occurred()) PyErr_Print();
            fprintf(stderr, "make_db()를 찾을 수 없거나 호출할 수 없습니다\n");
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "database.py를 불러올 수 없습니다\n");
    }

    Py_Finalize();
    return 0;
}
