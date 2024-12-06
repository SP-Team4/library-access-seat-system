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

        // lcd.py 모듈 불러오기
    PyObject *lcdModuleName = PyUnicode_DecodeFSDefault("lcd");
    PyObject *lcdModule = PyImport_Import(lcdModuleName);
    Py_DECREF(lcdModuleName);

    if (lcdModule != NULL) {
        // lcd_execute() 함수 실행
        PyObject *lcdFunc = PyObject_GetAttrString(lcdModule, "lcd_execute");
        if (lcdFunc && PyCallable_Check(lcdFunc)) {
            // 인자 생성
            PyObject *args = PyTuple_Pack(2, PyUnicode_DecodeFSDefault("First argument"), PyUnicode_DecodeFSDefault("Second argument"));
            if (args != NULL) {
                PyObject *pValue = PyObject_CallObject(lcdFunc, args);
                Py_DECREF(args);

                if (pValue != NULL) {
                    printf("lcd_execute() 실행 성공\n");
                    Py_DECREF(pValue);
                } else {
                    PyErr_Print();
                    fprintf(stderr, "lcd_execute() 실행 중 오류 발생\n");
                }
            }
        } else {
            if (PyErr_Occurred()) PyErr_Print();
            fprintf(stderr, "lcd_execute()를 찾을 수 없거나 호출할 수 없습니다\n");
        }
        Py_XDECREF(lcdFunc);
        Py_DECREF(lcdModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "lcd.py를 불러올 수 없습니다\n");
    }

    Py_Finalize();
    return 0;
}
