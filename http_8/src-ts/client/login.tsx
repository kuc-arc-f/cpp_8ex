import React, { useState , useEffect } from 'react';
import { Input } from '../components/Input';
import { Button } from '../components/Button';

const App: React.FC = () => {
  const [formData, setFormData] = useState({
    email: '',
    password: ''
  });
  const [errors, setErrors] = useState<{ email?: string; password?: string; general?: string }>({});
  const [isLoading, setIsLoading] = useState(false);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: value
    }));
    // Clear error when user starts typing
    if (errors[name as keyof typeof errors]) {
      setErrors(prev => ({ ...prev, [name]: undefined }));
    }
  };

  const validate = () => {
    const newErrors: typeof errors = {};
    if (!formData.email) {
      newErrors.email = 'メールアドレスを入力してください';
    } else if (!/\S+@\S+\.\S+/.test(formData.email)) {
      newErrors.email = '有効なメールアドレスを入力してください';
    }

    if (!formData.password) {
      newErrors.password = 'パスワードを入力してください';
    } else if (formData.password.length < 6) {
      newErrors.password = 'パスワードは6文字以上で入力してください';
    }

    return newErrors;
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setErrors({});
    
    const validationErrors = validate();
    if (Object.keys(validationErrors).length > 0) {
      setErrors(validationErrors);
      return;
    }

    setIsLoading(true);

    // Simulate API Call
    try {
      console.log(formData);
      const newTodo= {
        email: formData.email.trim(),
        password: formData.password.trim()
      };
      const response = await fetch('/api/login', {
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
      //cookie
      const d = new Date();
      d.setTime(d.getTime() + (365 * 24 * 60 * 60 * 1000));
      const expires = "expires=" + d.toUTCString();
      document.cookie = "http_8user=1;" + expires + ";path=/";
      //alert("Login successful!");
      location.href = "/";

      await new Promise(resolve => setTimeout(resolve, 1500));
      console.log('Login successful with:', formData);
      alert(`ログイン成功！\nEmail: ${formData.email}`);
    } catch (error) {
      setErrors({ general: 'ログインに失敗しました。時間をおいて再度お試しください。' });
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-slate-50 p-4 sm:px-6 lg:px-8">
      {/* Main Card */}
      <div className="max-w-md w-full space-y-8 bg-white p-8 rounded-2xl shadow-xl border border-slate-100">
        
        {/* Header */}
        <div className="text-center">
          <div className="mx-auto h-12 w-12 bg-blue-100 rounded-full flex items-center justify-center mb-4">
             <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6 text-blue-600">
              <path strokeLinecap="round" strokeLinejoin="round" d="M15.75 6a3.75 3.75 0 1 1-7.5 0 3.75 3.75 0 0 1 7.5 0ZM4.501 20.118a7.5 7.5 0 0 1 14.998 0A17.933 17.933 0 0 1 12 21.75c-2.676 0-5.216-.584-7.499-1.632Z" />
            </svg>
          </div>
          <h2 className="text-3xl font-bold text-slate-900 tracking-tight">
            アカウントにログイン
          </h2>
          <p className="mt-2 text-sm text-slate-600">
            サービスを利用するにはログインしてください
          </p>
        </div>

        {/* Form */}
        <form className="mt-8 space-y-6" onSubmit={handleSubmit} noValidate>
          <div className="space-y-4">
            <Input
              id="email"
              name="email"
              type="text"
              label="メールアドレス"
              placeholder="example@domain.com"
              value={formData.email}
              onChange={handleChange}
              error={errors.email}
              autoComplete="email"
            />
            
            <div className="space-y-1">
              <Input
                id="password"
                name="password"
                type="password"
                label="パスワード"
                placeholder="••••••••"
                value={formData.password}
                onChange={handleChange}
                error={errors.password}
                autoComplete="current-password"
              />
              <div className="flex items-center justify-end">
                <div className="text-sm">
                  <a href="#" className="font-medium text-blue-600 hover:text-blue-500 transition-colors">
                    パスワードをお忘れですか？
                  </a>
                </div>
              </div>
            </div>
          </div>

          {errors.general && (
             <div className="bg-red-50 border border-red-200 text-red-600 px-4 py-3 rounded-lg text-sm flex items-center">
               <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5 mr-2" viewBox="0 0 20 20" fill="currentColor">
                  <path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7 4a1 1 0 11-2 0 1 1 0 012 0zm-1-9a1 1 0 00-1 1v4a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
                </svg>
               {errors.general}
             </div>
          )}

          <div className="pt-2">
            <Button type="submit" isLoading={isLoading}>
              ログイン
            </Button>
          </div>
        </form>

        {/* Footer */}
        <div className="text-center">
          <p className="text-sm text-slate-600">
            アカウントをお持ちでないですか？{' '}
            <a href="#" className="font-medium text-blue-600 hover:text-blue-500 transition-colors">
              新規登録 (無料)
            </a>
          </p>
        </div>
      </div>
    </div>
  );
};

export default App;