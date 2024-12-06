#include <Python.h>

int call_python_function(const char *python_name, const char *python_function, const char *parameter) {
    // Python 인터프리터 초기화
    Py_Initialize();

    // sys.path에 현재 디렉터리 추가
    PyObject *sys_path = PySys_GetObject("path");
    PyList_Append(sys_path, PyUnicode_DecodeFSDefault("."));

    // Python 모듈 로드
    PyObject *pModule = PyImport_ImportModule(python_name);
    if (pModule == NULL) {
        PyErr_Print();
        Py_Finalize();
        return -1;  // 모듈 로드 실패
    }

    // Python 함수 가져오기
    PyObject *pFunc = PyObject_GetAttrString(pModule, python_function);
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        Py_Finalize();
        return -1;  // 함수 가져오기 실패
    }

    // 인자가 없으면 빈 튜플을 준비
    PyObject *pArgs;
    if (parameter != NULL) {
        pArgs = PyTuple_Pack(1, PyUnicode_DecodeFSDefault(parameter));  // 하나의 인자 전달
        if (pArgs == NULL) {
            PyErr_Print();
            Py_DECREF(pFunc);
            Py_DECREF(pModule);
            Py_Finalize();
            return -1;  // 인자 패킹 실패
        }
    } else {
        pArgs = PyTuple_New(0);  // 인자가 없을 경우 빈 튜플
        if (pArgs == NULL) {
            PyErr_Print();
            Py_DECREF(pFunc);
            Py_DECREF(pModule);
            Py_Finalize();
            return -1;  // 빈 튜플 생성 실패
        }
    }

    // 함수 호출
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    if (pValue == NULL) {
        PyErr_Print();
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        Py_Finalize();
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
        Py_Finalize();
        return -1;  // 변환 오류
    }

    // 메모리 해제
    Py_DECREF(pValue);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    // Python 인터프리터 종료
    Py_Finalize();

    printf("%d", result);
    return result;  // 반환값
}

int call_lcd(const char *parameter1, const char *parameter2) {

       // Python 인터프리터 초기화
    Py_Initialize();

    // sys.path에 현재 디렉터리 추가
    PyObject *sys_path = PySys_GetObject("path");
    PyList_Append(sys_path, PyUnicode_DecodeFSDefault("."));

    // lcd.py 모듈 불러오기
    PyObject *lcdModuleName = PyUnicode_DecodeFSDefault("lcd");
    PyObject *lcdModule = PyImport_Import(lcdModuleName);
    Py_DECREF(lcdModuleName);

    if (lcdModule != NULL) {
        // lcd_execute() 함수 실행
        PyObject *lcdFunc = PyObject_GetAttrString(lcdModule, "lcd_execute");
        if (lcdFunc && PyCallable_Check(lcdFunc)) {
            // 인자 생성
            PyObject *args = PyTuple_Pack(2, PyUnicode_DecodeFSDefault(parameter1), PyUnicode_DecodeFSDefault(parameter2));
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
    return 1;
}



int main() {
    //call_python_function("database","make_db", NULL );
    call_lcd("ajou", "library");
}
