
import React, { useState , useEffect } from 'react';
import { CheckCircle2, Circle, Trash2, Plus } from 'lucide-react';

interface Todo {
  id: string;
  title: string;
  completed: boolean;
}

export default function App() {
  const [todos, setTodos] = useState<Todo[]>([]);
  const [title, setTitle] = useState('');

  // TODOの取得
  useEffect(() => {
    const fetchTodos = async () => {
      try {
        const response = await fetch('/todos'); // デフォルトでGETになります
        if (!response.ok) {
          throw new Error(`エラーが発生しました: ${response.status}`);
        }        
        const data = await response.json();       
        console.log(data);
        setTodos(data);
      } catch (error) {
        console.error('Error fetching todos:', error);
      }
    };
    fetchTodos();
  }, []);  

  const handleAddTodo = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!title.trim()) return;
    
    const newTodo: Todo = {
      id: crypto.randomUUID(),
      title: title.trim(),
      completed: false,
    };
    console.log(newTodo)
    try {
      const response = await fetch('/todos', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(newTodo)
      });
      // レスポンスが 200~299 以外（404や500など）の場合
      if (!response.ok) {
        throw new Error(`HTTPエラー: ${response.status}`);
      }
      //const result = await response.json();
      const result = await response.text();
      console.log('成功:', result);  
    } catch (error) {
      // ネットワーク遮断やコードのミスなどのエラーをキャッチ
      console.error('通信に失敗しました:', error);
    }      
    setTodos([newTodo, ...todos]);
    setTitle('');
  };

  const toggleTodo = (id: string) => {
    setTodos(
      todos.map((todo) =>
        todo.id === id ? { ...todo, completed: !todo.completed } : todo
      )
    );
  };

  const deleteTodo =async (id: string) => {
    try {
      const response = await fetch("/todos/" + id, {
        method: 'DELETE', // メソッドを DELETE に指定
        // 認証が必要な場合はここに headers を追加します
      });
      if (!response.ok) {
        throw new Error(`削除に失敗しました: ${response.status}`);
      }
      // サーバーによっては空のレスポンスを返すこともあるので注意
      console.log(`${id}番のデータを削除しました`);
      setTodos(todos.filter((todo) => todo.id !== id));
    } catch (error) {
      console.error('エラー発生:', error);
    }
  };

  return (
    <div className="min-h-screen bg-slate-50 py-12 px-4 sm:px-6 lg:px-8 font-sans">
      <div className="max-w-md mx-auto bg-white rounded-2xl shadow-sm border border-slate-100 overflow-hidden">
        <div className="p-6">
          <h1 className="text-2xl font-semibold text-slate-900 mb-6">TODO List</h1>

          <form onSubmit={handleAddTodo} className="flex gap-2 mb-6">
            <input
              type="text"
              value={title}
              onChange={(e) => setTitle(e.target.value)}
              placeholder="What needs to be done?"
              className="flex-1 border border-slate-200 rounded-lg px-4 py-2 text-slate-900 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent transition-shadow"
            />
            <button
              type="submit"
              disabled={!title.trim()}
              className="bg-indigo-600 text-white px-4 py-2 rounded-lg hover:bg-indigo-700 disabled:opacity-50 disabled:cursor-not-allowed transition-colors flex items-center gap-2 font-medium"
            >
              <Plus size={20} />
              <span>Add</span>
            </button>
          </form>

          <div className="space-y-2">
            {todos.length === 0 ? (
              <p className="text-center text-slate-500 py-6">No tasks yet. Add one above!</p>
            ) : (
              todos.map((todo) => (
                <div
                  key={todo.id}
                  className="flex items-center justify-between p-3 rounded-lg hover:bg-slate-50 group transition-colors border border-transparent hover:border-slate-100"
                >
                  <div 
                    className="flex items-center gap-3 flex-1 cursor-pointer" 
                    onClick={() => toggleTodo(todo.id)}
                  >
                    <button className="text-slate-400 hover:text-indigo-600 transition-colors focus:outline-none">
                      {todo.completed ? (
                        <CheckCircle2 className="text-indigo-600" size={24} />
                      ) : (
                        <Circle size={24} />
                      )}
                    </button>
                    <span 
                      className={`flex-1 text-slate-700 transition-all ${
                        todo.completed ? 'line-through text-slate-400' : ''
                      }`}
                    >
                      {todo.title}
                    </span>
                  </div>
                  <button
                    onClick={() => deleteTodo(todo.id)}
                    className="text-slate-400 hover:text-red-500 opacity-0 group-hover:opacity-100 transition-all focus:outline-none focus:opacity-100 p-1"
                    aria-label="Delete todo"
                  >
                    <Trash2 size={18} />
                  </button>
                </div>
              ))
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
