#r "Newtonsoft.Json"
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Net;


public static async Task<HttpResponseMessage> Run(HttpRequestMessage req, TraceWriter log)
{
  double humidity;
  int rawTemp ;
  double Ctemp ;
  double Ftemp ;
  double voltage ;
  string utcEnque ;
  string utcProcess ;
  string connString = "https://api.powerbi.com/beta/************************";
  HttpClient client = new HttpClient();
  dynamic content = await req.Content.ReadAsStringAsync();
  Product product = new Product();
  log.Info("C# HTTP trigger function processed a request: " + content);
  
  JArray array = JArray.Parse($"{await req.Content.ReadAsStringAsync()}");
  
   foreach(dynamic message in array){
      humidity = ((message.humid1)*256 + (message.humid2))/100;
      rawTemp = ((message.temp1)*256 + (message.temp2));
      Ctemp = rawTemp /100.0;   
      Ftemp = Ctemp *1.8 + 32; 
      int bat = ((message.bat1)*256 + (message.bat2));
      voltage = 0.00322 * bat;
      utcProcess = message.EventProcessedUtcTime;
      utcEnque = message.EventEnqueuedUtcTime;
      log.Info(humidity.ToString());
      log.Info(Ctemp.ToString());
      log.Info(Ftemp.ToString());
      log.Info(voltage.ToString());
      log.Info(utcProcess);
      log.Info(utcEnque);
     
      product.Ctemperature = Ctemp;
      product.Ftemperature = Ftemp;
      product.humid = humidity;
      product.battery = voltage;
     //product.dateTime = ;   
      product.EventProcessedUtcTime=utcProcess; 
      product.EventEnqueuedUtcTime=utcEnque;
    
     }
    
     
     string output = JsonConvert.SerializeObject(product);

     HttpContent httpContent = new StringContent("[" + output + "]");
  
     HttpResponseMessage response = await client.PostAsync(connString, httpContent);

     response.EnsureSuccessStatusCode();
  
return req.CreateResponse(HttpStatusCode.OK, "Executed");

  } 

  

public class Message
{
    [JsonProperty("temp1")]
    public int temp1 { get; set; }
    [JsonProperty("temp2")]
    public int temp2 { get; set; }
    [JsonProperty("humid1")]
    public int humid1 { get; set; }
    [JsonProperty("humid2")]
    public int humid2 { get; set; }
    [JsonProperty("bat1")]
    public int bat1 { get; set; }
    [JsonProperty("bat2")]
    public int bat2 { get; set; }
  
}


public class Product{
  public double Ctemperature{get; set;}
  public double humid{get; set;}
  public double battery{get; set;}
  //public double dateTime{get; set;}
  public string EventProcessedUtcTime { get; set; }
  public string EventEnqueuedUtcTime { get; set; }
  public double Ftemperature{get; set;}
}