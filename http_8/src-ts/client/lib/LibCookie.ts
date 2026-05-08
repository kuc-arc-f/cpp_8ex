const LibCookie = {

  delete_cookie: function(key:string){
    try{
      document.cookie = `${key}=; path=/; expires=Thu, 01 Jan 1970 00:00:01 GMT`;
    } catch (e) {
      console.log(e);
      throw new Error('error, get_cookie');
    }
  },   
}
export default LibCookie;